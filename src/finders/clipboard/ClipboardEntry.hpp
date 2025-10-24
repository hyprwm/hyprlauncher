#pragma once

#include "../IFinder.hpp"
#include "../../clipboard/ClipboardManager.hpp"

class CClipboardEntry : public IFinderResult {
  public:
    CClipboardEntry(const SClipboardHistoryItem& item, CClipboardManager* manager);
    virtual ~CClipboardEntry() = default;

    virtual const std::string& fuzzable();
    virtual eFinderTypes       type();
    virtual void               run();
    virtual void               remove();

    std::string                m_Content;
    std::string                m_OriginalLine;
    std::string                m_ID;
    CClipboardManager*         m_Manager;
};
