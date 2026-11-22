#include "Search.hpp"
#include <glib.h>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <tuple>

namespace {
    std::string normalize(const std::string& text) {
        if (!g_utf8_validate(text.c_str(), text.size(), nullptr))
            return {};
        auto*       folded     = g_utf8_casefold(text.c_str(), text.size());
        auto*       normalized = g_utf8_normalize(folded, -1, G_NORMALIZE_DEFAULT_COMPOSE);
        std::string result     = normalized ? normalized : "";
        g_free(normalized);
        g_free(folded);
        const auto first = result.find_first_not_of(" \t\r\n");
        return first == std::string::npos ? "" : result.substr(first, result.find_last_not_of(" \t\r\n") - first + 1);
    }
    std::vector<gunichar> codepoints(const std::string& text) {
        std::vector<gunichar> result;
        for (const char* p = text.c_str(); *p; p = g_utf8_next_char(p))
            result.push_back(g_utf8_get_char(p));
        return result;
    }
    // Restricted Damerau-Levenshtein distance: edits and adjacent transpositions.
    int distance(const std::vector<gunichar>& a, const std::vector<gunichar>& b, int budget) {
        if (std::abs(int(a.size()) - int(b.size())) > budget)
            return budget + 1;
        std::vector<int> previous(b.size() + 1), older, current(b.size() + 1);
        std::iota(previous.begin(), previous.end(), 0);
        for (size_t i = 1; i <= a.size(); ++i) {
            current[0] = i;
            for (size_t j = 1; j <= b.size(); ++j) {
                current[j] = std::min({previous[j] + 1, current[j - 1] + 1, previous[j - 1] + (a[i - 1] != b[j - 1])});
                if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1])
                    current[j] = std::min(current[j], older[j - 2] + 1);
            }
            older = previous;
            previous.swap(current);
        }
        return previous.back();
    }
}

std::vector<SP<IFinderResult>> DesktopSearch::match(const std::vector<SP<IFinderResult>>& entries, const std::string& query, size_t limit) {
    const auto q = normalize(query);
    if (q.empty()) {
        auto sorted = entries;
        std::ranges::stable_sort(sorted, [](const auto& a, const auto& b) {
            if (a->frequency() != b->frequency())
                return a->frequency() > b->frequency();
            return normalize(a->name()) < normalize(b->name());
        });
        sorted.resize(std::min(limit, sorted.size()));
        return sorted;
    }
    struct Match {
        int               rank;
        std::string       name;
        SP<IFinderResult> entry;
    };
    std::vector<Match> literal, fuzzy;
    const auto         queryPoints = codepoints(q);
    const int          budget      = queryPoints.size() < 4 ? 0 : queryPoints.size() < 8 ? 1 : 2;
    for (const auto& entry : entries) {
        auto                     name = normalize(entry->name());
        int                      rank = name == q ? 0 : name.starts_with(q) ? 1 : name.contains(q) ? 2 : 4;
        std::vector<std::string> candidates{name};
        for (const auto& alias : entry->fuzzables()) {
            auto value = normalize(alias);
            if (!value.empty()) {
                if (value.contains(q))
                    rank = std::min(rank, 3);
                candidates.push_back(std::move(value));
            }
        }
        if (rank < 4) {
            literal.push_back({rank, std::move(name), entry});
            continue;
        }
        if (!budget)
            continue;
        int best = budget + 1;
        for (const auto& candidate : candidates) {
            best = std::min(best, distance(queryPoints, codepoints(candidate), budget));
            std::istringstream words(candidate);
            for (std::string word; words >> word;)
                best = std::min(best, distance(queryPoints, codepoints(word), budget));
        }
        if (best <= budget)
            fuzzy.push_back({best, std::move(name), entry});
    }
    auto& selected = literal.empty() ? fuzzy : literal;
    std::ranges::stable_sort(selected, [](const auto& a, const auto& b) { return std::tie(a.rank, a.name) < std::tie(b.rank, b.name); });
    std::vector<SP<IFinderResult>> result;
    for (size_t i = 0; i < std::min(limit, selected.size()); ++i)
        result.push_back(selected[i].entry);
    return result;
}
