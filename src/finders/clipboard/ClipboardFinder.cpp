#include "ClipboardFinder.hpp"

#include <algorithm>
#include "ClipboardEntry.hpp"
#include "../../helpers/Log.hpp"
#include "../../config/ConfigManager.hpp"
#include "../Cache.hpp"
#include "../Fuzzy.hpp"
#include <cctype>
#include <sstream>

CClipboardFinder::CClipboardFinder() = default;

void CClipboardFinder::onConfigReload() {
    Debug::log(LOG, "ClipboardFinder reloading config.");
    m_pClipboardManager   = makeUnique<CClipboardManager>(g_configManager->clipboardConfig);
    m_entryFrequencyCache = makeUnique<CEntryCache>("clipboard");
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
        allHistoryItems.emplace_back(makeShared<CClipboardEntry>(item, m_pClipboardManager.get(), m_entryFrequencyCache.get()));
    }

    auto                       fuzzed = Fuzzy::getNResults(allHistoryItems, query, MAX_RESULTS_PER_FINDER);

    std::vector<SFinderResult> results;
    results.reserve(fuzzed.size());

    for (const auto& f : fuzzed) {
        results.emplace_back(SFinderResult{
            .label  = f->fuzzable(),
            .result = f,
        });
    }

    Debug::log(TRACE, "Returning {} filtered results.", results.size());
    return results;
}
