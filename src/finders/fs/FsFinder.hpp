#pragma once

#include "../IFinder.hpp"

#include <filesystem>
#include <unordered_map>

class CFsEntry;

class CFsFinder : public IFinder {
  public:
    CFsFinder();
    virtual ~CFsFinder() noexcept;

    virtual std::vector<SFinderResult> getResultsForQuery(const std::string& query);
    virtual void                       init();

  private:
    std::vector<SP<CFsEntry>>                               m_fsEntryCache;
    std::vector<SP<IFinderResult>>                          m_fsEntryCacheGeneric;
    std::unordered_map<std::filesystem::path, SP<CFsEntry>> m_fsEntryCacheMap;
    std::unordered_map<int, SP<CFsEntry>>                   m_wdMap;
    int                                                     m_fd = -1;
    bool                                                    m_allowSymlink;

    void                                                    loadPath();
    void                                                    updateEntryCache();

    void                                                    cacheEntry(const std::filesystem::path& path);
    void                                                    uncacheEntry(const std::filesystem::path& path);

    friend class CFsEntry;
};

inline UP<CFsFinder> g_fsFinder;
