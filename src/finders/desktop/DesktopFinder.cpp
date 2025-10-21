#include "DesktopFinder.hpp"
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

static std::optional<std::string> readFileAsString(const std::string& path) {
    std::filesystem::path p(path);
    std::error_code ec;

    if (!std::filesystem::exists(p, ec) || ec)
        return std::nullopt;

    std::filesystem::path resolved = std::filesystem::canonical(p, ec);
    if (ec || !std::filesystem::exists(resolved, ec) || ec || !std::filesystem::is_regular_file(resolved, ec))
        return std::nullopt;

    std::ifstream file(resolved);
    if (!file.good())
        return std::nullopt;

    return trim(std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>())));
}

class CDesktopEntry : public IFinderResult {
  public:
    CDesktopEntry()          = default;
    virtual ~CDesktopEntry() = default;

    virtual std::string fuzzable() {
        return m_fuzzable;
    }

    virtual eFinderTypes type() {
        return FINDER_DESKTOP;
    }

    virtual uint32_t frequency() {
        return m_frequency;
    }

    virtual void run() {
        Debug::log(TRACE, "Running {}", m_exec);

        g_desktopFinder->m_entryFrequencyCache->incrementCachedEntry(m_fuzzable);
        m_frequency = g_desktopFinder->m_entryFrequencyCache->getCachedEntry(m_fuzzable);

        // replace all funky codes with nothing
        auto toExec = m_exec;
        replaceInString(toExec, "%U", "");
        replaceInString(toExec, "%f", "");
        replaceInString(toExec, "%F", "");
        replaceInString(toExec, "%u", "");
        replaceInString(toExec, "%i", "");
        replaceInString(toExec, "%c", "");
        replaceInString(toExec, "%k", "");
        replaceInString(toExec, "%d", "");
        replaceInString(toExec, "%D", "");
        replaceInString(toExec, "%N", "");
        replaceInString(toExec, "%n", "");

        CProcess proc("/bin/sh", {"-c", toExec});
        proc.runAsync();
    }

    std::string m_name, m_exec, m_icon, m_fuzzable;

    uint32_t    m_frequency = 0;
};

static constexpr std::array<const char*, 5> DESKTOP_ENTRY_PATHS = {"/usr/local/share/applications", "/usr/share/applications", "~/.local/share/applications",
                                                                   "/run/current-system/sw/share/applications",
                                                                   "~/.local/state/nix/profiles/home-manager/home-path/share/applications"};

//
static std::string resolvePath(std::string p) {
    if (p[0] != '~')
        return p;

    const auto HOME = getenv("HOME");

    if (!HOME)
        return "";

    return HOME + p.substr(1);
}

CDesktopFinder::CDesktopFinder() : m_inotifyFd(inotify_init()), m_entryFrequencyCache(makeUnique<CEntryCache>("desktop")) {
    ;
}

void CDesktopFinder::init() {
    recache();
    replantWatch();
}

void CDesktopFinder::onInotifyEvent() {
    recache();

    replantWatch();
}

void CDesktopFinder::recache() {
    m_desktopEntryPaths.clear();
    m_desktopEntryCache.clear();
    m_desktopEntryCacheGeneric.clear();

    for (const auto& PATH : DESKTOP_ENTRY_PATHS) {
        std::error_code ec;
        auto            it = std::filesystem::directory_iterator(resolvePath(PATH), ec);
        if (ec)
            continue;
        for (const auto& e : it) {
            if ((!e.is_regular_file(ec) && !e.is_symlink(ec)) || ec)
                continue;

            cacheEntry(e.path().string());
        }

        m_desktopEntryPaths.emplace_back(resolvePath(PATH));
    }
}

void CDesktopFinder::replantWatch() {
    for (const auto& w : m_watches) {
        inotify_rm_watch(m_inotifyFd.get(), w);
    }

    m_watches.clear();

    while (true) {
        pollfd pfd = {
            .fd     = m_inotifyFd.get(),
            .events = POLLIN,
        };

        poll(&pfd, 1, 0);

        if (!(pfd.revents & POLLIN))
            break;

        static char buf[1024];

        read(m_inotifyFd.get(), buf, 1023);
    }

    for (const auto& p : m_desktopEntryPaths) {
        m_watches.emplace_back(inotify_add_watch(m_inotifyFd.get(), p.c_str(), IN_MODIFY | IN_DONT_FOLLOW));
    }
}

void CDesktopFinder::cacheEntry(const std::string& path) {
    Debug::log(TRACE, "desktop: caching entry {}", path);

    const auto READ_RESULT = readFileAsString(path);

    if (!READ_RESULT)
        return;

    const auto& DATA = *READ_RESULT;

    auto        extract = [&DATA](const std::string_view what) -> std::string_view {
        size_t begins = DATA.find("\n" + std::string{what});

        if (begins == std::string::npos)
            return "";

        begins = DATA.find('=', begins);

        if (begins == std::string::npos)
            return "";

        begins += 1; // eat the equals
        while (begins < DATA.size() && std::isspace(DATA[begins])) {
            ++begins;
        }

        size_t ends = DATA.find("\n", begins + 1);

        if (!ends)
            return std::string_view{DATA}.substr(begins);

        return std::string_view{DATA}.substr(begins, ends - begins);
    };

    const auto NAME      = extract("Name");
    const auto ICON      = extract("Icon");
    const auto EXEC      = extract("Exec");
    const auto NODISPLAY = extract("NoDisplay") == "true";

    if (EXEC.empty() || NAME.empty() || NODISPLAY) {
        Debug::log(TRACE, "Skipping entry, empty name / exec / NoDisplay");
        return;
    }

    auto& e       = m_desktopEntryCache.emplace_back(makeShared<CDesktopEntry>());
    e->m_exec     = EXEC;
    e->m_icon     = ICON;
    e->m_name     = NAME;
    e->m_fuzzable = NAME;
    std::ranges::transform(e->m_fuzzable, e->m_fuzzable.begin(), ::tolower);
    e->m_frequency = m_entryFrequencyCache->getCachedEntry(e->m_fuzzable);
    m_desktopEntryCacheGeneric.emplace_back(e);

    Debug::log(TRACE, "Cached: {} with icon {} and exec line of \"{}\"", NAME, ICON, EXEC);
}

std::vector<SFinderResult> CDesktopFinder::getResultsForQuery(const std::string& query) {
    std::vector<SFinderResult> results;

    auto                       fuzzed = Fuzzy::getNResults(m_desktopEntryCacheGeneric, query, MAX_RESULTS_PER_FINDER);

    results.reserve(fuzzed.size());

    for (const auto& f : fuzzed) {
        const auto p = reinterpretPointerCast<CDesktopEntry>(f);
        if (!p)
            continue;
        results.emplace_back(SFinderResult{
            .label  = p->m_name,
            .icon   = p->m_icon,
            .result = p,
        });
    }

    return results;
}
