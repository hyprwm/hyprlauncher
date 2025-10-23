#include "ClipboardEntry.hpp"
#include "../../helpers/Log.hpp"

CClipboardEntry::CClipboardEntry(const SClipboardHistoryItem& item, CClipboardManager* manager) : m_Content(item.displayLine), m_OriginalLine(item.originalLine), m_Manager(manager) {}

const std::string& CClipboardEntry::fuzzable() {
    return m_OriginalLine;
}

eFinderTypes CClipboardEntry::type() {
    return FINDER_CLIPBOARD;
}

void CClipboardEntry::run() {
    Debug::log(LOG, "Running clipboard entry: copying and pasting");
    m_Manager->copyItem(m_OriginalLine);
}

void CClipboardEntry::remove() {
    Debug::log(LOG, "Removing clipboard entry from history");
    m_Manager->deleteItem(m_OriginalLine);
}
