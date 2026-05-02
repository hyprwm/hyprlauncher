#pragma once

#include "../IFinder.hpp"

#include <filesystem>

class CFsEntry;

class CFsFinder : public IFinder {
  public:
    CFsFinder();
    virtual ~CFsFinder() = default;

    virtual std::vector<SFinderResult> getResultsForQuery(const std::string& query);
    virtual void                       init();

  private:
    std::vector<SP<CFsEntry>>      m_fsEntryCache;
    std::vector<SP<IFinderResult>> m_fsEntryCacheGeneric;

    void                           cacheEntry(const std::filesystem::path& path);

    friend class CFsEntry;
};

inline UP<CFsFinder> g_fsFinder;
