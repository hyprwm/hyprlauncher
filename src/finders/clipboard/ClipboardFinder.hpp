#pragma once

#include "../IFinder.hpp"
#include "../../clipboard/ClipboardManager.hpp"
#include <memory>

class CClipboardFinder : public IFinder {
  public:
    virtual ~CClipboardFinder() = default;
    CClipboardFinder();

    virtual std::vector<SFinderResult> getResultsForQuery(const std::string& query);

    void clearCache();

    void onConfigReload();

  private:
    std::unique_ptr<CClipboardManager> m_pClipboardManager;

    std::vector<SFinderResult> m_vCachedHistory;
};

inline UP<CClipboardFinder> g_clipboardFinder;
