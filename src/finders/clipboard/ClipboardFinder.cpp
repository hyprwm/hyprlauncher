#include "ClipboardFinder.hpp"

#include <algorithm>
#include "ClipboardEntry.hpp"
#include "../../helpers/Log.hpp"
#include "../../config/ConfigManager.hpp"
#include <cctype>
#include <sstream>

CClipboardFinder::CClipboardFinder() = default;

void CClipboardFinder::onConfigReload() {
    Debug::log(LOG, "ClipboardFinder reloading config.");
    m_pClipboardManager = makeUnique<CClipboardManager>(g_configManager->clipboardConfig);
}


std::vector<SFinderResult> CClipboardFinder::getResultsForQuery(const std::string& query) {
    if (!m_pClipboardManager) {
        Debug::log(ERR, "Clipboard manager not initialized. Cannot get history.");
        return {};
    }

    const auto HISTORY = m_pClipboardManager->getHistory();
    Debug::log(TRACE, "Fetched history has {} items.", HISTORY.size());

    std::vector<CSharedPointer<IFinderResult>> allHistoryItems;
    allHistoryItems.reserve(HISTORY.size());
    for (const auto& item : HISTORY) {
        allHistoryItems.emplace_back(makeShared<CClipboardEntry>(item, m_pClipboardManager.get()));
    }

    std::vector<SFinderResult> results;

    if (query.empty()) {
        results.reserve(allHistoryItems.size());
        for (const auto& item : allHistoryItems) {
            results.emplace_back(SFinderResult{
                .label  = item->fuzzable(),
                .result = item,
            });
        }
    } else {
        std::string lowerQuery = query;
        std::ranges::transform(lowerQuery, lowerQuery.begin(), [](unsigned char c) { return std::tolower(c); });

        const auto strBegin = lowerQuery.find_first_not_of(" \t");
        if (strBegin == std::string::npos) // No non-whitespace
            return {};
        const auto strEnd = lowerQuery.find_last_not_of(" \t");
        const auto strRange = strEnd - strBegin + 1;
        lowerQuery = lowerQuery.substr(strBegin, strRange);

        std::stringstream queryStream(lowerQuery);
        std::string token;
        std::vector<std::string> queryTokens;
        while (queryStream >> token) {
            queryTokens.push_back(token);
        }

        results.reserve(allHistoryItems.size());
        for (const auto& item : allHistoryItems) {
            std::string lowerItem = item->fuzzable();
            std::ranges::transform(lowerItem, lowerItem.begin(), [](unsigned char c) { return std::tolower(c); });

            bool allTokensMatch = true;
            for (const auto& qToken : queryTokens) {
                if (lowerItem.find(qToken) == std::string::npos) {
                    allTokensMatch = false;
                    break;
                }
            }

            if (allTokensMatch) {
                results.emplace_back(SFinderResult{
                    .label  = item->fuzzable(),
                    .result = item,
                });
            }
        }
    }

    Debug::log(TRACE, "Returning {} filtered results.", results.size());
    return results;
}
