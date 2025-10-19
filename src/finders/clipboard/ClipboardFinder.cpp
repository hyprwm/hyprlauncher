#include "ClipboardFinder.hpp"
#include "ClipboardEntry.hpp"
#include "../../helpers/Log.hpp"
#include "../../config/ConfigManager.hpp"

CClipboardFinder::CClipboardFinder() = default;

void CClipboardFinder::onConfigReload() {
    Debug::log(LOG, "ClipboardFinder reloading config.");
    m_pClipboardManager = std::make_unique<CClipboardManager>(g_configManager->clipboardConfig);
    clearCache();
}

void CClipboardFinder::clearCache() {
    Debug::log(TRACE, "Clearing clipboard cache");
    m_vCachedHistory.clear();
}

std::vector<SFinderResult> CClipboardFinder::getResultsForQuery(const std::string& query) {
    if (!m_pClipboardManager) {
        Debug::log(ERR, "Clipboard manager not initialized. Cannot get history.");
        return {};
    }

    if (m_vCachedHistory.empty()) {
        Debug::log(LOG, "Clipboard cache empty and prefix detected. Fetching new history.");

        const auto HISTORY = m_pClipboardManager->getHistory();

        Debug::log(LOG, "Fetched history has {} items.", HISTORY.size());

        for (const auto& item : HISTORY) {
            m_vCachedHistory.push_back(SFinderResult{
                .label  = item,
                .result = makeShared<CClipboardEntry>(item, m_pClipboardManager.get())
            });
        }
    }

    return m_vCachedHistory;
}

