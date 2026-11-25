#include "Search.hpp"
#include <unicode/normalizer2.h>
#include <unicode/unistr.h>
#include <unicode/utf8.h>
#include <limits>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <tuple>

namespace {
    std::string normalize(const std::string& text) {
        if (text.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
            return {};
        const auto LENGTH = static_cast<int32_t>(text.size());
        // ICU's fromUTF8 replaces malformed input; validate first to preserve
        // the existing behavior of rejecting invalid desktop-entry strings.
        for (int32_t i = 0; i < LENGTH;) {
            UChar32 point = 0;
            U8_NEXT(text.data(), i, LENGTH, point);
            if (point <= 0)
                return {};
        }
        UErrorCode  status     = U_ZERO_ERROR;
        const auto* normalizer = icu::Normalizer2::getNFCInstance(status);
        if (U_FAILURE(status) || !normalizer)
            return {};
        auto folded = icu::UnicodeString::fromUTF8(icu::StringPiece(text.data(), LENGTH));
        folded.foldCase();
        icu::UnicodeString normalized;
        normalizer->normalize(folded, normalized, status);
        if (U_FAILURE(status))
            return {};
        std::string result;
        normalized.toUTF8String(result);
        const auto first = result.find_first_not_of(" \t\r\n");
        return first == std::string::npos ? "" : result.substr(first, result.find_last_not_of(" \t\r\n") - first + 1);
    }
    std::vector<UChar32> codepoints(const std::string& text) {
        std::vector<UChar32> result;
        const auto           LENGTH = static_cast<int32_t>(text.size());
        for (int32_t i = 0; i < LENGTH;) {
            UChar32 point = 0;
            U8_NEXT(text.data(), i, LENGTH, point);
            result.push_back(point);
        }
        return result;
    }
    // Restricted Damerau-Levenshtein distance: edits and adjacent transpositions.
    int distance(const std::vector<UChar32>& a, const std::vector<UChar32>& b, int budget) {
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
