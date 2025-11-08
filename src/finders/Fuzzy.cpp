#include "Fuzzy.hpp"
#include <algorithm>
#include <thread>

#include <unistd.h>

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

    return (sc<float>(matches) / LENGTH_A + sc<float>(matches) / LENGTH_B + (matches - t) / sc<float>(matches)) / 3.F;
}

constexpr const float BOOST_THRESHOLD = 0.65F;
constexpr const float FREQ_SCALE      = 0.03F;
constexpr const float PREFIX_SCALE    = 0.05F;
constexpr const float SUBSTR_SCALE    = 0.2F;

//
static float jaroWinklerFull(const std::string_view& query, const std::string_view& test, float freq) {
    float score = jaroWinkler(query, test);

    // if the similarity is good enough, we can consider substr and prefix.
    if (score > BOOST_THRESHOLD || test.contains(query)) {
        size_t       prefixLen = 0;
        const size_t MAXPREFIX = 20;

        while (prefixLen < std::min({query.size(), test.size(), MAXPREFIX}) && query[prefixLen] == test[prefixLen]) {
            ++prefixLen;
        }

        if (test.contains(query))
            score += (std::min(test.length(), sc<size_t>(4)) * SUBSTR_SCALE * (1.F - score));
        if (prefixLen)
            score += (sc<float>(prefixLen) * PREFIX_SCALE * (1.F - score));

        score += freq * FREQ_SCALE * (1.F - score);
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