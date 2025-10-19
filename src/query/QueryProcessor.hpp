#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>

#include "../helpers/Memory.hpp"

class IFinder;

class CQueryProcessor {
  public:
    CQueryProcessor();
    ~CQueryProcessor();

    void scheduleQueryUpdate(const std::string& str);
    void overrideQueryProvider(WP<IFinder> finder);

  private:
    std::condition_variable m_threadCV;
    std::mutex              m_threadMutex, m_queryStrMutex, m_processingMutex;
    std::thread             m_queryThread;
    std::string             m_pendingQuery;
    bool                    m_quit = false, m_event = false;
    WP<IFinder>             m_overrideFinder;
};

inline UP<CQueryProcessor> g_queryProcessor = makeUnique<CQueryProcessor>();