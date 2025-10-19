#pragma once

#include "../IFinder.hpp"
#include "../../clipboard/ClipboardManager.hpp"
#include "../Cache.hpp"

class CClipboardFinder : public IFinder {
  public:
    virtual ~CClipboardFinder() = default;
    CClipboardFinder();

    virtual std::vector<SFinderResult> getResultsForQuery(const std::string& query);
    void                               onConfigReload();

  private:
    UP<CClipboardManager> m_pClipboardManager;
    UP<CEntryCache>       m_entryFrequencyCache;
};

inline UP<CClipboardFinder> g_clipboardFinder;
