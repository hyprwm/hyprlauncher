#include "Fuzzy.hpp"
#include <algorithm>
#include <cmath>
#include <ranges>
#include <thread>

#include <unistd.h>

#include <hyprutils/string/String.hpp>

using namespace Hyprutils::String;

namespace {

    class CVarListView : public std::ranges::view_interface<CVarListView> {
        std::string_view m_str;
        char             m_sep;

      public:
        // (removeEmpty = true)
        CVarListView(std::string_view str, char sep) : m_str(str), m_sep(sep) {}

        class CIterator {
          private:
            std::string_view m_str;
            std::size_t      m_index = -1uz;
            std::size_t      m_count = 0;
            char             m_sep;

          public:
            using iterator_category = std::forward_iterator_tag;
            using value_type        = std::string_view;
            using difference_type   = std::ptrdiff_t;
            using reference         = value_type;
            using pointer           = value_type;

            CIterator() = default;
            CIterator(std::string_view str, char sep) : m_str(str), m_sep(sep) {
                this->operator++();
            }

            reference operator*() const {
                return m_str.substr(m_index, m_count);
            }

            pointer operator->() const {
                return this->operator*();
            }

            CIterator& operator++() {
                m_index += m_count;
                do {
                    m_index += 1;
                    if (m_index >= m_str.size()) {
                        m_index = -1uz;
                        return *this;
                    }
                } while (isSep(m_str[m_index]));

                m_count = 1;

                while (m_index + m_count < m_str.size() && !isSep(m_str[m_index + m_count]))
                    m_count += 1;

                return *this;
            }

            CIterator operator++(int) {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }

            bool isSep(char c) const {
                return m_sep == ' ' ? std::isspace(c) : m_sep == '/' ? std::isspace(c) || c == m_sep : c == m_sep;
            }

            bool operator==(const CIterator& other) const {
                return m_index == other.m_index;
            }
        };

        using iterator = CIterator;

        auto begin() const {
            return CIterator(m_str, m_sep);
        }
        auto end() const {
            return CIterator();
        }
    };

}

static float jaroWinkler(const std::string_view& query, const std::string_view& test) {
    const auto LENGTH_A = query.length();
    const auto LENGTH_B = test.length();

    if (!LENGTH_A && !LENGTH_B)
        return 0;

    const auto MATCH_DISTANCE = LENGTH_A == 1 && LENGTH_B == 1 ? 0 : ((std::max(LENGTH_A, LENGTH_B) / 2) - 1);

    bool*      matchesA = (bool*)alloca(LENGTH_A * sizeof(bool));
    bool*      matchesB = (bool*)alloca(LENGTH_B * sizeof(bool));
    size_t     matches  = 0;
    for (size_t i = 0; i < LENGTH_A; ++i) {
        const size_t start = (i > MATCH_DISTANCE ? i - MATCH_DISTANCE : 0);
        const size_t end   = std::min(i + MATCH_DISTANCE + 1, LENGTH_B);
        for (size_t j = start; j < end; ++j) {
            if (matchesB[j] || query[i] != test[j])
                continue;
            matchesA[i] = true;
            matchesB[j] = true;
            ++matches;
            break;
        }
    }

    if (!matches)
        return 0.F;

    float  t = 0.F;
    size_t k = 0;
    for (size_t i = 0; i < LENGTH_A; ++i) {
        if (!matchesA[i])
            continue;

        while (k < LENGTH_B && !matchesB[k]) {
            ++k;
        }

        if (query[i] != test[k])
            t += 0.5F;

        ++k;
    }

    float jaro = (sc<float>(matches) / LENGTH_A + sc<float>(matches) / LENGTH_B + (matches - t) / sc<float>(matches)) / 3.F;

    // winkler prefix bonus
    size_t prefixLen = 0;
    size_t maxPrefix = std::min({LENGTH_A, LENGTH_B, sc<size_t>(4)});
    for (size_t i = 0; i < maxPrefix; ++i) {
        if (query[i] == test[i])
            ++prefixLen;
        else
            break;
    }

    return jaro + (prefixLen * 0.1F * (1.0F - jaro));
}

//
constexpr float MIN_FUZZY_TO_COUNT = 0.75F;
constexpr float MIN_SALIENT_MATCH  = 0.3F;
constexpr float MIN_TOKEN_MATCH    = 0.15F;
constexpr float POPULARITY_FACTOR  = 0.08F;
constexpr float NO_SALIENT_PENALTY = 0.01F;
constexpr float EXACT_MATCH_SCORE  = 2.0F;

//
static float tokenBestMatch(std::string_view qt, std::string_view lastQ, const CVarListView& cTok) {
    if (qt.empty())
        return 0.F;

    float best             = 0.F;
    bool  hasExplicitMatch = false; // prefix or substring match

    for (auto ct : cTok) {
        if (ct == qt)
            return 1.F;

        // strong prefix match - especially important for the last token (partial typing)
        if (ct.starts_with(qt)) {
            hasExplicitMatch = true;
            if (!lastQ.empty() && qt == lastQ) {
                // last token prefix match - user is still typing
                best = std::max(best, 0.95F);
            } else
                best = std::max(best, 0.98F);
        } else if (ct.contains(qt)) {
            hasExplicitMatch = true;
            best             = std::max(best, 0.7F);
        }

        best = std::max(best, jaroWinkler(qt, ct));
    }

    // if we have an explicit match (prefix/substring), return the score directly
    if (hasExplicitMatch)
        return best;

    // for pure fuzzy matches, require minimum quality
    if (best < MIN_FUZZY_TO_COUNT)
        return 0.F;
    return (best - MIN_FUZZY_TO_COUNT) / (1.F - MIN_FUZZY_TO_COUNT);
}

static float scoreCandidate(const std::vector<std::string_view>& qTokens, std::string_view queryLowerTrim, const std::string& query, std::string_view cand, float freq,
                            char tokenBreak) {
    const float popFactor = 1.F + (POPULARITY_FACTOR * std::log1p(std::max(0.F, freq)));

    // exact matches occupy a reserved band above any achievable fuzzy score, so a popular
    // partial match can never outrank them; popularity only orders matches within a band
    if (queryLowerTrim == trim(cand))
        return EXACT_MATCH_SCORE + popFactor;

    CVarListView cTok(cand, tokenBreak);

    if (qTokens.empty() || cTok.begin() == cTok.end())
        return 0.F;

    std::string_view lastQ = qTokens.back();

    // pick salient token as longest
    std::string_view salient      = qTokens[0];
    float            salientMatch = tokenBestMatch(qTokens[0], lastQ, cTok);
    float            sum          = 0.F + salientMatch;
    float            minMatch     = std::min(1.F, salientMatch);
    for (auto qt : qTokens | std::views::drop(1)) {
        float match = tokenBestMatch(qt, lastQ, cTok);
        sum += match;
        minMatch = std::min(minMatch, match);

        if (qt.size() > salient.size()) {
            salient      = qt;
            salientMatch = match;
        }
    }

    // if ANY token matches poorly, penalize heavily
    if (minMatch < MIN_TOKEN_MATCH)
        return 0.F;

    float base = sum / sc<float>(qTokens.size()); // normalize it

    // if salient token doesn't match strongly, kill the score
    if (salientMatch < MIN_SALIENT_MATCH)
        base *= NO_SALIENT_PENALTY;

    float lenDiff   = float(std::abs(int(query.size()) - int(cand.size())));
    float lenFactor = std::exp(-lenDiff / 25.f);

    return base * lenFactor * popFactor;
}

struct SScoreData {
    float             score = 0.F;
    SP<IFinderResult> result;
    size_t            idx = 0;
};

static void workerFn(std::vector<SScoreData>& scores, const std::vector<SP<IFinderResult>>& in, const std::vector<std::string_view>& qTokens, const std::string& queryLowerTrim,
                     const std::string& query, size_t start, size_t end, char tokenBreak) {
    for (size_t i = start; i < end; ++i) {
        auto& ref = scores[i];

        float bestScore = 0.F;
        for (auto const& candidate : in[i]->fuzzables()) {
            auto score = scoreCandidate(qTokens, queryLowerTrim, query, candidate, in[i]->frequency(), tokenBreak);
            bestScore  = std::max(score, bestScore);
        }
        ref.score = bestScore;

        ref.result = in[i];
        ref.idx    = i;
    }
}

static std::vector<SP<IFinderResult>> getBestResultsStable(std::vector<SScoreData>& data, size_t n) {
    std::vector<SP<IFinderResult>> resVec;
    resVec.resize(std::min(data.size(), n));

    static auto getBestResult = [](std::vector<SScoreData>& data) -> typename std::vector<SScoreData>::iterator {
        typename std::vector<SScoreData>::iterator result    = data.begin();
        float                                      bestScore = -1.F;
        for (auto it = data.begin(); it != data.end(); ++it) {
            if (it->score > bestScore && it->score >= 0.F) {
                bestScore = it->score;
                result    = it;
            }
        }

        return result;
    };

    for (size_t i = 0; i < std::min(data.size(), n); ++i) {
        auto it   = getBestResult(data);
        it->score = -1.F; // reset, don't get it again
        resVec[i] = it->result;
    }

    return resVec;
}

static constexpr const decltype(sysconf(0)) MAX_THREADS = 10;

//
std::vector<SP<IFinderResult>> Fuzzy::getNResults(const std::vector<SP<IFinderResult>>& in, const std::string& query, size_t results, char tokenBreak) {
    std::string queryLowerTrim{query};
    std::ranges::transform(queryLowerTrim, queryLowerTrim.begin(), ::tolower);
    queryLowerTrim = trim(queryLowerTrim);

    auto                    qTokens = CVarListView(query, tokenBreak) | std::ranges::to<std::vector>();

    std::vector<SScoreData> scores;
    scores.resize(in.size());

    if (in.size() > 100) {
        // If we have more than 100 elements, we can run this in threads.
        // For smaller sets this doesn't make much sense
        // Value 100 was picked because I felt like it's a good one™.
        auto THREADS = sysconf(_SC_NPROCESSORS_ONLN);
        if (THREADS < 1)
            THREADS = 8;
        THREADS = std::min(THREADS, MAX_THREADS);

        std::vector<std::thread> workerThreads;
        workerThreads.resize(THREADS);
        size_t workElDone = 0, workElPerThread = in.size() / THREADS;
        for (long i = 0; i < THREADS; ++i) {
            if (i == THREADS - 1) {
                workerThreads[i] = std::thread([&, begin = workElDone] { workerFn(scores, in, qTokens, queryLowerTrim, query, begin, in.size(), tokenBreak); });
                break;
            }
            workerThreads[i] =
                std::thread([&, begin = workElDone, end = workElDone + workElPerThread] { workerFn(scores, in, qTokens, queryLowerTrim, query, begin, end, tokenBreak); });

            workElDone += workElPerThread;
        }

        for (auto& t : workerThreads) {
            if (t.joinable())
                t.join();
        }

        workerThreads.clear();
    } else
        workerFn(scores, in, qTokens, queryLowerTrim, query, 0, in.size(), tokenBreak);

    return getBestResultsStable(scores, results);
}

// NOLINTNEXTLINE SHUT THE FUCK UP
std::vector<std::string> Fuzzy::createFuzzableStrings(std::vector<std::string_view>&& strings, bool toLowercase) {
    std::vector<std::string> fuzzables{};
    fuzzables.reserve(strings.size());

    for (auto&& sv : strings) {
        std::string fuzzable;
        fuzzable.resize(sv.size());

        if (toLowercase)
            std::ranges::transform(sv, fuzzable.begin(), ::tolower);
        else
            fuzzable.assign(sv);

        fuzzables.emplace_back(std::move(fuzzable));
    }

    return fuzzables;
}
