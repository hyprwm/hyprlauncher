#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <cstdint>
#include <vector>

#include "../helpers/Memory.hpp"

class IFinder;
struct SFinderResult;

class CQueryProcessor {
  public:
    CQueryProcessor();
    ~CQueryProcessor();

    void scheduleQueryUpdate(const std::string& str);
    void overrideQueryProvider(IFinder* finder);
    void selectQueryProvider(const std::string& finder);

  private:
    struct SQueryRequest {
        std::string query;
        IFinder*    finder     = nullptr;
        uint64_t    generation = 0;
    };

    void                         process(SQueryRequest&& request);
    void                         publishResults(uint64_t generation, std::vector<SFinderResult>&& results);
    bool                         isCurrentGeneration(uint64_t generation);

    std::condition_variable      m_threadCV;
    std::mutex                   m_mutex, m_processingMutex;
    std::thread                  m_queryThread;
    std::optional<SQueryRequest> m_pendingQuery;
    IFinder*                     m_overrideFinder = nullptr;
    IFinder*                     m_selectFinder   = nullptr;
    uint64_t                     m_generation     = 0;
    bool                         m_quit           = false;
};

inline UP<CQueryProcessor> g_queryProcessor = makeUnique<CQueryProcessor>();
