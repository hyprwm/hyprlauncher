#include "Fuzzy.hpp"
#include <algorithm>
#include <thread>

#include <unistd.h>

static float jaroWinkler(const std::string_view& a, const std::string_view& b) {
    const auto LENGTH_A = a.length();
    const auto LENGTH_B = b.length();

    if (!LENGTH_A && !LENGTH_B)
        return 0;

    const auto        MATCH_DISTANCE = LENGTH_A == 1 && LENGTH_B == 1 ? 0 : ((std::max(LENGTH_A, LENGTH_B) / 2) - 1);

    std::vector<bool> matchesA, matchesB;
    matchesA.resize(LENGTH_A);
    matchesB.resize(LENGTH_B);
    size_t matches = 0;
    for (size_t i = 0; i < LENGTH_A; ++i) {
        const auto END = std::min(MATCH_DISTANCE + i + 1, LENGTH_B);
        for (size_t j = (i > MATCH_DISTANCE ? (i - MATCH_DISTANCE) : 0); j < END; ++j) {
            if (matchesB[j] || a[i] != b[j])
                continue;

            matchesA[i] = true;
            matchesB[j] = true;
            ++matches;
        }
    }

    if (!matches)
        return 0;

    float  t = 0.F;
    size_t k = 0;
    for (size_t i = 0; i < LENGTH_A; ++i) {
        if (!matchesA[i])
            continue;

        while (k < matchesB.size() && !matchesB[k]) {
            ++k;
        }

        if (a[i] != b[k])
            t += 0.5F;

        ++k;
    }

    return (sc<float>(matches) / LENGTH_A + sc<float>(matches) / LENGTH_B + (matches - t) / sc<float>(matches)) / 3.F;
}

constexpr const float BOOST_THRESHOLD = 0.69F;
constexpr const float FREQ_SCALE      = 0.05F;
constexpr const float PREFIX_SCALE    = 0.1F;
constexpr const float SUBSTR_SCALE    = 0.05F;

//
static float jaroWinklerFull(const std::string_view& a, const std::string_view& b, float freq) {
    float score = jaroWinkler(a, b);

    // if the similarity is good enough, we can consider substr and prefix.
    if (score > BOOST_THRESHOLD) {
        size_t       prefixLen = 0;
        const size_t MAXPREFIX = 4;

        while (prefixLen < std::min({a.size(), b.size(), MAXPREFIX}) && a[prefixLen] == b[prefixLen]) {
            ++prefixLen;
        }

        score += freq * FREQ_SCALE;

        if (b.contains(a))
            return score + (std::min(b.length(), sc<size_t>(4)) * SUBSTR_SCALE);
        return score + (sc<float>(prefixLen) * PREFIX_SCALE);
    }

    return score;
}

struct SScoreData {
    float             score = 0.F;
    SP<IFinderResult> result;
    size_t            idx = 0;
};

static void workerFn(std::vector<SScoreData>& scores, const std::vector<SP<IFinderResult>>& in, const std::string& query, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        auto& ref  = scores[i];
        ref.score  = jaroWinklerFull(query, in[i]->fuzzable(), in[i]->frequency());
        ref.result = in[i];
        ref.idx    = i;
    }
}

static std::vector<SP<IFinderResult>> getBestResultsStable(std::vector<SScoreData>& data, size_t n) {
    std::vector<SP<IFinderResult>> resVec;
    resVec.resize(std::min(data.size(), n));

    static auto getBestResult = [](std::vector<SScoreData>& data) -> typename std::vector<SScoreData>::iterator {
        typename std::vector<SScoreData>::iterator result    = data.begin();
        float                                      bestScore = 0.F;
        for (auto it = data.begin(); it != data.end(); ++it) {
            if (it->score > bestScore) {
                bestScore = it->score;
                result    = it;
            }
        }

        return result;
    };

    for (size_t i = 0; i < n; ++i) {
        auto it   = getBestResult(data);
        it->score = 0.F; // reset, don't get it again
        resVec[i] = it->result;
    }

    return resVec;
}

static constexpr const decltype(sysconf(0)) MAX_THREADS = 10;

//
std::vector<SP<IFinderResult>> Fuzzy::getNResults(const std::vector<SP<IFinderResult>>& in, const std::string& query, size_t results) {
    std::vector<SScoreData> scores;
    scores.resize(in.size());

    // to analyze scores, run this op in parallel
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
        } else
            workerThreads[i] = std::thread([&, begin = workElDone, end = workElDone + workElPerThread] { workerFn(scores, in, query, begin, end); });

        workElDone += workElPerThread;
    }

    for (auto& t : workerThreads) {
        if (t.joinable())
            t.join();
    }

    workerThreads.clear();

    return getBestResultsStable(scores, results);
}