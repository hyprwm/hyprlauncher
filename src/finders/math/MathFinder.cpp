#include "MathFinder.hpp"
#include "../../helpers/Log.hpp"

#include <hyprutils/os/Process.hpp>
#include <libqalculate/qalculate.h>

using namespace Hyprutils::OS;

static UP<Calculator> qalculator;

class CMathEntry : public IFinderResult {
  public:
    CMathEntry()          = default;
    virtual ~CMathEntry() = default;

    virtual std::string fuzzable() {
        return "";
    }

    virtual eFinderTypes type() {
        return FINDER_MATH;
    }

    virtual void run() {
        Debug::log(TRACE, "Copying {} with wl-copy", m_result);

        CProcess proc("wl-copy", {m_result});
        proc.runAsync();
    }

    std::string m_expr, m_result;
};

CMathFinder::CMathFinder() = default;

std::vector<SFinderResult> CMathFinder::getResultsForQuery(const std::string& query) {
    if (!qalculator) {
        qalculator = makeUnique<Calculator>();
        qalculator->loadExchangeRates();
        qalculator->loadGlobalDefinitions();
        qalculator->loadLocalDefinitions();
    }

    SFinderResult result;
    auto          entry = makeShared<CMathEntry>();
    entry->m_expr       = query;
    entry->m_result     = qalculator->calculateAndPrint(query, 1500);
    result.result       = entry;
    result.label        = "= " + entry->m_result;
    return {result};
}
