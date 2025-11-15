#pragma once

#include "../IFinder.hpp"

#include <hyprutils/os/FileDescriptor.hpp>
#include <filesystem>

class CDesktopEntry;
class CEntryCache;

class CDesktopFinder : public IFinder {
  public:
    CDesktopFinder();
    virtual ~CDesktopFinder() = default;

    virtual std::vector<SFinderResult> getResultsForQuery(const std::string& query);
    virtual void                       init();

    Hyprutils::OS::CFileDescriptor     m_inotifyFd;

    void                               onInotifyEvent();

  private:
    std::vector<SP<CDesktopEntry>>     m_desktopEntryCache;
    std::vector<SP<IFinderResult>>     m_desktopEntryCacheGeneric;

    std::vector<std::filesystem::path> m_desktopEntryPaths;
    std::vector<int>                   m_watches;

    std::vector<std::filesystem::path> m_envPaths;

    UP<CEntryCache>                    m_entryFrequencyCache;

    void                               cacheEntry(const std::filesystem::path& path);
    void                               replantWatch();
    void                               recache();

    friend class CDesktopEntry;
};

inline UP<CDesktopFinder> g_desktopFinder;
