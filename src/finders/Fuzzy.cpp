#include "Fuzzy.hpp"
#include <algorithm>
#include <cmath>
#include <thread>
#include <unordered_set>

#include <unistd.h>

#include <hyprutils/string/VarList2.hpp>

using namespace Hyprutils::String;

static float jaroWinkler(const std::string_view& query, const std::string_view& test) {
    const auto LENGTH_A = query.length();
    const auto LENGTH_B = test.length();

    if (!LENGTH_A && !LENGTH_B)
        return 0;

    const auto        MATCH_DISTANCE = LENGTH_A == 1 && LENGTH_B == 1 ? 0 : ((std::max(LENGTH_A, LENGTH_B) / 2) - 1);

    std::vector<bool> matchesA, matchesB;
    matchesA.resize(LENGTH_A);
    matchesB.resize(LENGTH_B);
    size_t matches = 0;
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

        while (k < matchesB.size() && !matchesB[k]) {
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

//
static float tokenBestMatch(std::string_view qt, std::string_view lastQ, const std::unordered_set<std::string_view>& cset, const std::vector<std::string_view>& cTok) {
    if (qt.empty())
        return 0.F;
    if (cset.contains(qt))
        return 1.F;

    float best             = 0.F;
    bool  hasExplicitMatch = false; // prefix or substring match

    for (auto ct : cTok) {
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

static float scoreCandidate(std::string_view query, std::string_view cand, float freq) {
    CVarList2                     qTokens(std::string{query}, 0, 's', true, false);
    CVarList2                     cTokens(std::string{cand}, 0, 's', true, false);

    std::vector<std::string_view> qTok, cTok;
    qTok.reserve(qTokens.size());
    cTok.reserve(cTokens.size());
    for (const auto& q : qTokens) {
        qTok.emplace_back(q);
    }
    for (const auto& c : cTokens) {
        cTok.emplace_back(c);
    }

    if (qTok.empty() || cTok.empty())
        return 0.F;

    std::unordered_set<std::string_view> cset;
    cset.reserve(cTok.size());
    for (auto t : cTok) {
        cset.insert(t);
    }

    std::string_view lastQ = qTok.back();

    // pick salient token as longest
    std::string_view salient = qTok[0];
    for (auto t : qTok) {
        if (t.size() > salient.size())
            salient = t;
    }

    float sum      = 0.F;
    float minMatch = 1.F;
    for (auto qt : qTok) {
        float match = tokenBestMatch(qt, lastQ, cset, cTok);
        sum += match;
        minMatch = std::min(minMatch, match);
    }

    // if ANY token matches poorly, penalize heavily
    if (minMatch < MIN_TOKEN_MATCH)
        return 0.F;

    float base = sum / sc<float>(qTok.size()); // normalize it

    // if salient token doesn't match strongly, kill the score
    float salientMatch = tokenBestMatch(salient, lastQ, cset, cTok);
    if (salientMatch < MIN_SALIENT_MATCH)
        base *= NO_SALIENT_PENALTY;

    float lenDiff   = float(std::abs(int(query.size()) - int(cand.size())));
    float lenFactor = std::exp(-lenDiff / 25.f);

    float popFactor = 1.F + (POPULARITY_FACTOR * std::log1p(std::max(0.F, freq)));

    return base * lenFactor * popFactor;
}

struct SScoreData {
    float             score = 0.F;
    SP<IFinderResult> result;
    size_t            idx = 0;
};

static void workerFn(std::vector<SScoreData>& scores, const std::vector<SP<IFinderResult>>& in, const std::string& query, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        auto& ref = scores[i];

        ref.score = scoreCandidate(query, in[i]->fuzzable(), in[i]->frequency());

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
std::vector<SP<IFinderResult>> Fuzzy::getNResults(const std::vector<SP<IFinderResult>>& in, const std::string& query, size_t results) {
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
                workerThreads[i] = std::thread([&, begin = workElDone] { workerFn(scores, in, query, begin, in.size()); });
                break;
            }
            workerThreads[i] = std::thread([&, begin = workElDone, end = workElDone + workElPerThread] { workerFn(scores, in, query, begin, end); });

            workElDone += workElPerThread;
        }

        for (auto& t : workerThreads) {
            if (t.joinable())
                t.join();
        }

        workerThreads.clear();
    } else
        workerFn(scores, in, query, 0, in.size());

    return getBestResultsStable(scores, results);
}