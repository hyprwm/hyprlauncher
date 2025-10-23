#include "ClipboardFinder.hpp"
#include "ClipboardEntry.hpp"
#include "../../helpers/Log.hpp"
#include "../../config/ConfigManager.hpp"
#include "../Fuzzy.hpp"

CClipboardFinder::CClipboardFinder() = default;

void CClipboardFinder::onConfigReload() {
    Debug::log(LOG, "ClipboardFinder reloading config.");
    m_ClipboardManager   = makeUnique<CClipboardManager>(g_configManager->m_clipboardConfig);
}

std::vector<SFinderResult> CClipboardFinder::getResultsForQuery(const std::string& query) {
    if (!m_ClipboardManager) {
        Debug::log(ERR, "Clipboard manager not initialized. Cannot get history.");
        return {};
    }

    const auto HISTORY = m_ClipboardManager->getHistory();
    Debug::log(TRACE, "Fetched history has {} items.", HISTORY.size());
    std::vector<SFinderResult> results;

    if (query.empty()) {
        const size_t limit = std::min((size_t)MAX_RESULTS_PER_FINDER, HISTORY.size());
        results.reserve(limit);
        for (size_t i = 0; i < limit; ++i) {
            const auto& item = HISTORY[i];
            results.emplace_back(SFinderResult{
                .label  = item.displayLine,
                .result = makeShared<CClipboardEntry>(item, m_ClipboardManager.get()),
            });
        }
    } else {
        std::vector<CSharedPointer<IFinderResult>> allHistoryItems;
        allHistoryItems.reserve(HISTORY.size());
        for (const auto& item : HISTORY) {
            allHistoryItems.emplace_back(makeShared<CClipboardEntry>(item, m_ClipboardManager.get()));
        }

        auto fuzzed = Fuzzy::getNResults(allHistoryItems, query, MAX_RESULTS_PER_FINDER);
        results.reserve(fuzzed.size());

        for (const auto& f : fuzzed) {
            const auto entry = reinterpretPointerCast<CClipboardEntry>(f);
            if (!entry)
                continue;

            results.emplace_back(SFinderResult{
                .label  = entry->m_Content,
                .result = f,
            });
        }
    }

    Debug::log(TRACE, "Returning {} filtered results.", results.size());
    return results;
}
