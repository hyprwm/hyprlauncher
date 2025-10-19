#include "IPCFinder.hpp"
#include "../../helpers/Log.hpp"
#include "../Fuzzy.hpp"
#include "../Cache.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sys/inotify.h>
#include <sys/poll.h>

#include <hyprutils/string/String.hpp>
#include <hyprutils/os/Process.hpp>

using namespace Hyprutils::String;
using namespace Hyprutils::OS;

class CIPCEntry : public IFinderResult {
  public:
    CIPCEntry()          = default;
    virtual ~CIPCEntry() = default;

    virtual std::string fuzzable() {
        return m_entry;
    }

    virtual eFinderTypes type() {
        return FINDER_IPC;
    }

    virtual uint32_t frequency() {
        return 0;
    }

    virtual void run() {
        Debug::log(TRACE, "Selected {}", m_entry);
    }

    std::string m_entry;
};

CIPCFinder::CIPCFinder() = default;

void CIPCFinder::init() {
    ;
}

void CIPCFinder::setData(const std::vector<const char*>& data) {
    m_entries.clear();
    m_entriesGeneric.clear();
    for (const auto& s : data) {
        auto e = m_entries.emplace_back(makeShared<CIPCEntry>());
        m_entriesGeneric.emplace_back(e);
        e->m_entry = s;
    }
}

void CIPCFinder::setData(const std::vector<std::string>& data) {
    m_entries.clear();
    m_entriesGeneric.clear();
    for (const auto& s : data) {
        auto e = m_entries.emplace_back(makeShared<CIPCEntry>());
        m_entriesGeneric.emplace_back(e);
        e->m_entry = s;
    }
}

std::vector<SFinderResult> CIPCFinder::getResultsForQuery(const std::string& query) {
    std::vector<SFinderResult>     results;

    std::vector<SP<IFinderResult>> fuzzed;
    if (!query.empty())
        fuzzed = Fuzzy::getNResults(m_entriesGeneric, query, MAX_RESULTS_PER_FINDER);
    else
        fuzzed = std::vector<SP<IFinderResult>>{m_entriesGeneric.begin(), m_entriesGeneric.begin() + std::min(m_entriesGeneric.size(), MAX_RESULTS_PER_FINDER)};

    results.reserve(fuzzed.size());

    for (const auto& f : fuzzed) {
        const auto p = reinterpretPointerCast<CIPCEntry>(f);
        if (!p)
            continue;
        results.emplace_back(SFinderResult{
            .label  = p->m_entry,
            .icon   = "",
            .result = p,
        });
    }

    return results;
}
