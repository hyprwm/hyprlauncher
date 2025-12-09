#pragma once

#include "../IFinder.hpp"

class CFontEntry;

class CFontFinder : public IFinder {
  public:
    CFontFinder();
    virtual ~CFontFinder();

    virtual std::vector<SFinderResult> getResultsForQuery(const std::string& query);

  private:
    std::vector<SP<CFontEntry>>    m_entries;
    std::vector<SP<IFinderResult>> m_entriesGeneric;

    void                           refreshFonts();

    bool                           m_valid = false;
};

inline UP<CFontFinder> g_fontFinder;
