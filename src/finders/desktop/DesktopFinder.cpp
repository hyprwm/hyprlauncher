#include "DesktopFinder.hpp"
#include "../../helpers/Log.hpp"
#include "../Fuzzy.hpp"
#include "../Cache.hpp"
#include "../../config/ConfigManager.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <unistd.h>
#include <unordered_set>

#include <hyprutils/string/String.hpp>
#include <hyprutils/os/Process.hpp>
#include <hyprutils/string/ConstVarList.hpp>
#include <hyprutils/string/VarList2.hpp>

using namespace Hyprutils::String;
using namespace Hyprutils::OS;

static std::optional<std::string> readFileAsString(const std::filesystem::path& path) {
    std::error_code ec;

    if (!std::filesystem::exists(path, ec) || ec)
        return std::nullopt;

    std::ifstream file(path.string());
    if (!file.good())
        return std::nullopt;

    return trim(std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>())));
}

class CDesktopEntry : public IFinderResult {
  public:
    CDesktopEntry()          = default;
    virtual ~CDesktopEntry() = default;

    virtual const std::vector<std::string>& fuzzables() {
        return m_fuzzables;
    }

    virtual eFinderTypes type() {
        return FINDER_DESKTOP;
    }

    virtual uint32_t frequency() {
        return m_frequency;
    }

    virtual const std::string& name() {
        return m_name;
    }

    virtual void run() {
        static auto            PLAUNCHPREFIX = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:desktop_launch_prefix");
        static auto            PTERMINALEXEC = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:desktop_terminal");
        const std::string_view LAUNCH_PREFIX = *PLAUNCHPREFIX;
        const std::string_view TERMINAL_EXEC = *PTERMINALEXEC;

        auto                   toExec = std::format("{}{}{}", LAUNCH_PREFIX.empty() ? std::string{""} : std::string{LAUNCH_PREFIX} + std::string{" "},
                                                    m_terminal && !TERMINAL_EXEC.empty() ? std::string{TERMINAL_EXEC} + std::string{" "} : std::string{""}, m_exec);

        Debug::log(TRACE, "Running {}", toExec);

        g_desktopFinder->m_entryFrequencyCache->incrementCachedEntry(m_name);
        m_frequency = g_desktopFinder->m_entryFrequencyCache->getCachedEntry(m_name);

        // replace all funky codes with nothing
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

    std::string              m_name, m_exec, m_icon, m_stem;
    std::vector<std::string> m_fuzzables;
    bool                     m_terminal = false;

    uint32_t                 m_frequency = 0;
};

static std::filesystem::path resolvePath(const std::string& p) {
    if (p[0] != '~')
        return p;

    const auto HOME = getenv("HOME");

    if (!HOME)
        return "";

    return std::filesystem::path(HOME) / p.substr(2);
}

using SymlinkWatchMap = std::unordered_map<std::filesystem::path, std::unordered_set<std::string>>;

static SymlinkWatchMap collectSymlinkWatches(const std::filesystem::path& path) {
    constexpr size_t MAX_SYMLINKS = 40;
    SymlinkWatchMap  watches;
    std::error_code  ec;
    auto             pending = std::filesystem::absolute(path, ec).lexically_normal();

    if (ec)
        return watches;

    for (size_t followed = 0; followed < MAX_SYMLINKS; ++followed) {
        auto prefix       = pending.root_path();
        bool foundSymlink = false;

        for (auto it = pending.begin(); it != pending.end(); ++it) {
            if (*it == pending.root_path())
                continue;

            prefix /= *it;

            const auto status = std::filesystem::symlink_status(prefix, ec);
            if (ec)
                return watches;
            if (!std::filesystem::is_symlink(status))
                continue;

            watches[prefix.parent_path()].emplace(prefix.filename().string());

            const auto target = std::filesystem::read_symlink(prefix, ec);
            if (ec)
                return watches;

            std::filesystem::path suffix;
            for (++it; it != pending.end(); ++it)
                suffix /= *it;

            pending      = (target.is_absolute() ? target : prefix.parent_path() / target) / suffix;
            pending      = pending.lexically_normal();
            foundSymlink = true;
            break;
        }

        if (!foundSymlink)
            break;
    }

    return watches;
}

CDesktopFinder::CDesktopFinder() : m_inotifyFd(inotify_init()), m_entryFrequencyCache(makeUnique<CEntryCache>("desktop")) {
    if (const auto DATA_HOME = getenv("XDG_DATA_HOME"))
        m_envPaths.emplace_back(std::filesystem::path(DATA_HOME) / "applications");
    else
        m_envPaths.emplace_back(resolvePath("~/.local/share/applications"));

    if (const auto DATA_DIRS = getenv("XDG_DATA_DIRS")) {
        CConstVarList paths(DATA_DIRS, 0, ':', false);
        for (const auto& p : paths)
            m_envPaths.emplace_back(std::filesystem::path(p) / "applications");
    } else {
        m_envPaths.emplace_back("/usr/local/share/applications");
        m_envPaths.emplace_back("/usr/share/applications");
    }
}

void CDesktopFinder::init() {
    recache();
    replantWatch();
}

void CDesktopFinder::onInotifyEvent() {
    alignas(inotify_event) std::array<char, 16 * 1024> buffer        = {};
    bool                                               shouldRecache = false;

    while (true) {
        pollfd pfd = {
            .fd     = m_inotifyFd.get(),
            .events = POLLIN,
        };

        if (poll(&pfd, 1, 0) <= 0 || !(pfd.revents & POLLIN))
            break;

        const auto bytesRead = read(m_inotifyFd.get(), buffer.data(), buffer.size());
        if (bytesRead <= 0)
            break;

        for (size_t offset = 0; offset + sizeof(inotify_event) <= sc<size_t>(bytesRead);) {
            const auto* event     = rc<const inotify_event*>(buffer.data() + offset);
            const auto  eventSize = sizeof(inotify_event) + event->len;
            if (offset + eventSize > sc<size_t>(bytesRead))
                break;

            if (event->mask & IN_Q_OVERFLOW)
                shouldRecache = true;

            if (m_contentWatches.contains(event->wd))
                shouldRecache = true;

            if (const auto rootWatch = m_rootWatchNames.find(event->wd); rootWatch != m_rootWatchNames.end()) {
                if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_IGNORED))
                    shouldRecache = true;
                else if (event->len && rootWatch->second.contains(event->name))
                    shouldRecache = true;
            }

            offset += eventSize;
        }
    }

    if (!shouldRecache)
        return;

    recache();
    replantWatch();
}

void CDesktopFinder::recache() {
    m_desktopEntryPaths.clear();
    m_desktopEntryCache.clear();
    m_desktopEntryCacheGeneric.clear();

    std::unordered_set<std::string>                                                 desktopFileIds;
    std::unordered_set<std::filesystem::path>                                       directories;

    std::function<void(const std::filesystem::path&, const std::filesystem::path&)> cacheDirectory;
    cacheDirectory = [this, &cacheDirectory, &desktopFileIds, &directories](const std::filesystem::path& base, const std::filesystem::path& p) {
        std::error_code ec;
        auto            canonicalPath = std::filesystem::canonical(p, ec);
        if (ec || !directories.insert(canonicalPath).second) {
            Debug::log(TRACE, "desktop: skipping {}, does not exist / already visited", p.string());
            return;
        }
        auto it = std::filesystem::directory_iterator(p, ec);
        if (ec)
            return;
        for (const auto& e : it) {
            auto status = e.status(ec);
            if (ec)
                continue;
            if (std::filesystem::is_regular_file(status)) {
                auto relDesktopFilePath = e.path().lexically_relative(base);
                if (relDesktopFilePath.extension() != ".desktop") {
                    Debug::log(TRACE, "desktop: skipping non-desktop file at {}", e.path().string());
                    continue;
                }
                auto desktopFileId = relDesktopFilePath.string();
                std::ranges::replace(desktopFileId, '/', '-');
                if (desktopFileIds.insert(desktopFileId).second)
                    cacheEntry(e.path());
                else
                    Debug::log(TRACE, "desktop: skipping entry at {}, already cached desktopFileId {}", e.path().string(), desktopFileId);
            } else if (std::filesystem::is_directory(status))
                cacheDirectory(base, e.path());
        }

        m_desktopEntryPaths.emplace_back(p);
    };

    for (const auto& PATH : m_envPaths) {
        cacheDirectory(PATH, PATH);
    }
}

void CDesktopFinder::replantWatch() {
    for (const auto& w : m_watches) {
        inotify_rm_watch(m_inotifyFd.get(), w);
    }

    m_watches.clear();
    m_contentWatches.clear();
    m_rootWatchNames.clear();

    while (true) {
        pollfd pfd = {
            .fd     = m_inotifyFd.get(),
            .events = POLLIN,
        };

        if (poll(&pfd, 1, 0) <= 0 || !(pfd.revents & POLLIN))
            break;

        std::array<char, 1024> buffer = {};
        if (read(m_inotifyFd.get(), buffer.data(), buffer.size()) <= 0)
            break;
    }

    std::unordered_set<int> watches;
    const auto              addWatch = [this, &watches](const std::filesystem::path& path, uint32_t mask) -> int {
        const int watch = inotify_add_watch(m_inotifyFd.get(), path.c_str(), mask | IN_MASK_ADD);
        if (watch < 0) {
            Debug::log(WARN, "desktop: failed to watch {}", path.string());
            return -1;
        }

        watches.emplace(watch);
        return watch;
    };

    constexpr uint32_t CONTENT_MASK = IN_MODIFY | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF;
    for (const auto& path : m_desktopEntryPaths) {
        const int watch = addWatch(path, CONTENT_MASK);
        if (watch >= 0)
            m_contentWatches.emplace(watch);
    }

    SymlinkWatchMap symlinkWatches;
    for (const auto& path : m_envPaths) {
        for (auto&& [parent, names] : collectSymlinkWatches(path))
            symlinkWatches[parent].insert(names.begin(), names.end());
    }

    constexpr uint32_t ROOT_MASK = IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF;
    for (const auto& [parent, names] : symlinkWatches) {
        const int watch = addWatch(parent, ROOT_MASK | IN_DONT_FOLLOW);
        if (watch >= 0)
            m_rootWatchNames[watch].insert(names.begin(), names.end());
    }

    m_watches.assign(watches.begin(), watches.end());
}

void CDesktopFinder::cacheEntry(const std::filesystem::path& path) {
    Debug::log(TRACE, "desktop: caching entry at {}", path.string());

    const auto READ_RESULT = readFileAsString(path);

    if (!READ_RESULT)
        return;

    const auto& DATA = *READ_RESULT;

    auto        extract = [&DATA](const std::string_view what) -> std::string_view {
        size_t begins = DATA.find("\n" + std::string{what} + " ");

        if (begins == std::string::npos)
            begins = DATA.find("\n" + std::string{what} + "=");

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
    const auto GEN_NAME  = extract("GenericName");
    const auto ICON      = extract("Icon");
    const auto EXEC      = extract("Exec");
    const auto NODISPLAY = extract("NoDisplay") == "true";
    const auto TERMINAL  = extract("Terminal") == "true";

    if (EXEC.empty() || NAME.empty() || NODISPLAY) {
        Debug::log(TRACE, "desktop: skipping entry, empty name / exec / NoDisplay");
        return;
    }

    auto pathStem = path.stem().string();

    if (path.string().starts_with("/home")) {
        // home paths should override system ones
        std::erase_if(m_desktopEntryCache, [&pathStem](const auto& e) { return e->m_stem == pathStem; });
    }

    auto& e        = m_desktopEntryCache.emplace_back(makeShared<CDesktopEntry>());
    e->m_exec      = EXEC;
    e->m_icon      = ICON;
    e->m_name      = NAME;
    e->m_stem      = std::move(pathStem);
    e->m_terminal  = TERMINAL;
    e->m_frequency = m_entryFrequencyCache->getCachedEntry(e->m_name);

    // create fuzzable strings. Read: name, generic name, but also keywords.
    std::vector<std::string_view> strings  = {NAME, GEN_NAME};
    const std::string_view        KEYWORDS = extract("Keywords");
    if (!KEYWORDS.empty()) {
        CVarList2 keywords(KEYWORDS, 0, ';', true);
        for (const auto& k : keywords) {
            strings.emplace_back(k);
        }

        // we need to emplace here because CVarList2 will be destroyed and the string_view
        // refs will become uafs
        e->m_fuzzables = Fuzzy::createFuzzableStrings(std::move(strings));
    } else
        e->m_fuzzables = Fuzzy::createFuzzableStrings(std::move(strings));

    m_desktopEntryCacheGeneric.emplace_back(e);

    Debug::log(TRACE, "desktop: cached {} with icon {} and exec line of \"{}\"", NAME, ICON, EXEC);
}

std::vector<SFinderResult> CDesktopFinder::getResultsForQuery(const std::string& query) {
    static auto                PICONSENABLED = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(g_configManager->m_config.get(), "finders:desktop_icons");

    std::vector<SFinderResult> results;

    if (query.empty()) {
        // Return all entries sorted by frequency (most used first)
        auto sorted = m_desktopEntryCacheGeneric;
        std::stable_sort(sorted.begin(), sorted.end(), [](const SP<IFinderResult>& a, const SP<IFinderResult>& b) { return a->frequency() > b->frequency(); });

        size_t count = std::min(sorted.size(), sc<size_t>(MAX_RESULTS_PER_FINDER));
        results.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            const auto p = reinterpretPointerCast<CDesktopEntry>(sorted[i]);
            if (!p)
                continue;
            results.emplace_back(SFinderResult{
                .label   = p->m_name,
                .icon    = *PICONSENABLED ? p->m_icon : "",
                .result  = p,
                .hasIcon = true,
            });
        }

        return results;
    }

    auto fuzzed = Fuzzy::getNResults(m_desktopEntryCacheGeneric, query, MAX_RESULTS_PER_FINDER);

    results.reserve(fuzzed.size());

    for (const auto& f : fuzzed) {
        const auto p = reinterpretPointerCast<CDesktopEntry>(f);
        if (!p)
            continue;
        results.emplace_back(SFinderResult{
            .label   = p->m_name,
            .icon    = *PICONSENABLED ? p->m_icon : "",
            .result  = p,
            .hasIcon = true,
        });
    }

    return results;
}
