#include "ClipboardEntry.hpp"
#include "../../helpers/Log.hpp"

CClipboardEntry::CClipboardEntry(const SClipboardHistoryItem& item, CClipboardManager* manager) : m_sContent(item.display_line), m_sOriginalLine(item.original_line), m_pManager(manager) {}

std::string CClipboardEntry::fuzzable() {
    return m_sOriginalLine;
}

eFinderTypes CClipboardEntry::type() {
    return FINDER_CLIPBOARD;
}

void CClipboardEntry::run() {
    Debug::log(LOG, "Running clipboard entry: copying and pasting");
    m_pManager->copyItem(m_sOriginalLine);
}

void CClipboardEntry::remove() {
    Debug::log(LOG, "Removing clipboard entry from history");
    m_pManager->deleteItem(m_sOriginalLine);
}
