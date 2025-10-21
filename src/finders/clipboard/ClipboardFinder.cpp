#include "ClipboardFinder.hpp"
#include "ClipboardEntry.hpp"
#include "../../helpers/Log.hpp"
#include "../../config/ConfigManager.hpp"
#include "../Fuzzy.hpp"

CClipboardFinder::CClipboardFinder() = default;

void CClipboardFinder::onConfigReload() {
    Debug::log(LOG, "ClipboardFinder reloading config.");
    m_pClipboardManager   = makeUnique<CClipboardManager>(g_configManager->clipboardConfig);
}

std::vector<SFinderResult> CClipboardFinder::getResultsForQuery(const std::string& query) {
    if (!m_pClipboardManager) {
        Debug::log(ERR, "Clipboard manager not initialized. Cannot get history.");
        return {};
    }

    const auto HISTORY = m_pClipboardManager->getHistory();
    Debug::log(TRACE, "Fetched history has {} items.", HISTORY.size());
    std::vector<SFinderResult> results;

    if (query.empty()) {
        const size_t limit = std::min((size_t)MAX_RESULTS_PER_FINDER, HISTORY.size());
        results.reserve(limit);
        for (size_t i = 0; i < limit; ++i) {
            const auto& item = HISTORY[i];
            results.emplace_back(SFinderResult{
                .label  = item.display_line,
                .result = makeShared<CClipboardEntry>(item, m_pClipboardManager.get()),
            });
        }
    } else {
        std::vector<CSharedPointer<IFinderResult>> allHistoryItems;
        allHistoryItems.reserve(HISTORY.size());
        for (const auto& item : HISTORY) {
            allHistoryItems.emplace_back(makeShared<CClipboardEntry>(item, m_pClipboardManager.get()));
        }

        auto fuzzed = Fuzzy::getNResults(allHistoryItems, query, MAX_RESULTS_PER_FINDER);
        results.reserve(fuzzed.size());

        for (const auto& f : fuzzed) {
            const auto pEntry = reinterpretPointerCast<CClipboardEntry>(f);
            if (!pEntry)
                continue;

            results.emplace_back(SFinderResult{
                .label  = pEntry->m_sContent,
                .result = f,
            });
        }
    }

    Debug::log(TRACE, "Returning {} filtered results.", results.size());
    return results;
}
