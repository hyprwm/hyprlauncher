#include "FsFinder.hpp"

#include "../Fuzzy.hpp"

#include <hyprutils/string/String.hpp>
#include <hyprutils/os/Process.hpp>
#include <hyprutils/string/ConstVarList.hpp>

#include <unordered_set>
#include <algorithm>

using namespace Hyprutils::String;
using namespace Hyprutils::OS;

class CFsEntry : public IFinderResult {
  public:
    CFsEntry()          = default;
    virtual ~CFsEntry() = default;

    virtual const std::string& fuzzable() {
        return m_fuzzable;
    }

    virtual eFinderTypes type() {
        return FINDER_FS;
    }

    virtual uint32_t frequency() {
        return m_frequency;
    }

    virtual const std::string& name() {
        return m_name;
    }

    virtual void run() {
        CProcess proc("xdg-open", {m_name});
        proc.runAsync();
    }

    std::string m_name, m_fuzzable;

    uint32_t    m_frequency = 0;
};

CFsFinder::CFsFinder() = default;

void CFsFinder::cacheEntry(const std::filesystem::path& path) {
    auto x        = m_fsEntryCache.emplace_back(makeShared<CFsEntry>());
    x->m_name     = path.string();
    x->m_fuzzable = x->m_name;
    std::ranges::transform(x->m_fuzzable, x->m_fuzzable.begin(), ::tolower);
    m_fsEntryCacheGeneric.emplace_back(std::move(x));
}

void CFsFinder::init() {
    // walk the fs from a config'd path
    // FIXME: config!!!
    const auto                                BASE_PATH = getenv("HOME");
    std::unordered_set<std::filesystem::path> visitedDirs;

    //
    std::function<void(const std::filesystem::path&)> walk = [&](const std::filesystem::path& p) -> void {
        if (visitedDirs.contains(p))
            return;

        std::error_code ec;
        const auto      CAN = std::filesystem::canonical(p, ec);

        if (ec)
            return;

        // cache dir
        visitedDirs.emplace(CAN);
        cacheEntry(CAN);

        for (const auto& e : std::filesystem::directory_iterator(CAN, ec)) {
            if (ec)
                break;

            if (e.is_regular_file()) {
                // cache file and continue
                cacheEntry(e);
                continue;
            }

            if (e.is_directory()) {
                // enter directory
                walk(e);
                continue;
            }

            // invalid... thing?
        }
    };

    walk(BASE_PATH);
}

std::vector<SFinderResult> CFsFinder::getResultsForQuery(const std::string& query) {
    std::vector<SFinderResult> results;

    auto                       fuzzed = Fuzzy::getNResults(m_fsEntryCacheGeneric, query, MAX_RESULTS_PER_FINDER, '/');

    results.reserve(fuzzed.size());

    for (const auto& f : fuzzed) {
        const auto p = reinterpretPointerCast<CFsEntry>(f);
        if (!p)
            continue;
        results.emplace_back(SFinderResult{
            .label   = p->m_name,
            .icon    = "",
            .result  = p,
            .hasIcon = false,
        });
    }

    return results;
}