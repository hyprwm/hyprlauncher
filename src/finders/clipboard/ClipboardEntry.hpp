#pragma once

#include "../IFinder.hpp"
#include "../../clipboard/ClipboardManager.hpp"

class CClipboardEntry : public IFinderResult {
  public:
    CClipboardEntry(const SClipboardHistoryItem& item, CClipboardManager* manager);
    virtual ~CClipboardEntry() = default;

    virtual std::string     fuzzable();
    virtual eFinderTypes    type();
    virtual void            run();
    virtual void            remove();

    std::string             m_sContent;
    std::string             m_sOriginalLine;
    CClipboardManager*      m_pManager;
};

