#include "FsFinder.hpp"

#include "../../helpers/Log.hpp"
#include "../Fuzzy.hpp"
#include "../../config/ConfigManager.hpp"

#include <cstring>
#include <algorithm>
#include <ranges>
#include <hyprutils/string/String.hpp>
#include <hyprutils/os/Process.hpp>
#include <hyprutils/string/ConstVarList.hpp>
#include <regex>
#include <unistd.h>
#include <sys/inotify.h>

using namespace Hyprutils::String;
using namespace Hyprutils::OS;

class CFsEntry : public IFinderResult {
  public:
    CFsEntry()          = default;
    virtual ~CFsEntry() = default;

    virtual const std::vector<std::string>& fuzzables() {
        return m_fuzzables;
    }

    virtual eFinderTypes type() {
        return FINDER_FS;
    }

    virtual uint32_t frequency() {
        return m_frequency;
    }

    virtual const std::string& name() {
        return m_path;
    }

    virtual void run() {
        CProcess proc("xdg-open", {m_path});
        proc.runAsync();
    }

    std::string              m_path;
    std::vector<std::string> m_fuzzables;

    uint32_t                 m_frequency = 0;
    int                      m_wd;
};

CFsFinder::CFsFinder() = default;
CFsFinder::~CFsFinder() noexcept {
    if (m_fd != -1)
        if (close(m_fd) != 0)
            Debug::log(ERR, "fs: failed to close file descriptor: {}", std::strerror(errno));
}

void CFsFinder::updateEntryCache() {
    if (m_fd == -1)
        return;

    alignas(alignof(inotify_event)) std::byte buffer[64 * 1024];

    while (true) {
        ssize_t length = read(m_fd, buffer, sizeof(buffer));

        if (length == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            if (errno == EINTR)
                continue;

            Debug::log(ERR, "fs: file descriptor error: {}", std::strerror(errno));
            break;
        }

        for (std::byte* it = buffer; it < buffer + length; it += sizeof(inotify_event) + reinterpret_cast<inotify_event*>(it)->len) {
            auto* event = reinterpret_cast<inotify_event*>(it);

            auto  wd_it = m_wdMap.find(event->wd);
            if (wd_it == m_wdMap.end())
                continue;

            std::filesystem::path path = wd_it->second->m_path;

            if (event->mask & (IN_CREATE | IN_MOVED_TO))
                cacheEntry(path / event->name);
            if (event->mask & (IN_DELETE | IN_MOVED_FROM))
                uncacheEntry(path / event->name);
            if (event->mask & (IN_DELETE_SELF | IN_MOVE_SELF))
                uncacheEntry(path);
        }
    }
}

void CFsFinder::cacheEntry(const std::filesystem::path& path) {
    std::error_code ec;
    const auto      CAN = std::filesystem::canonical(path, ec);
    if (ec)
        return;

    if (m_fsEntryCacheMap.contains(CAN))
        return;

    int wd = -1;
    if (std::filesystem::is_directory(CAN) && m_fd != -1) {
        wd = inotify_add_watch(m_fd, CAN.c_str(), IN_CREATE | IN_MOVED_TO | IN_DELETE | IN_MOVED_FROM | IN_DELETE_SELF | IN_MOVE_SELF);
        if (wd == -1)
            Debug::log(ERR, "fs: failed to add watch descriptor for '{}': {}", CAN.c_str(), std::strerror(errno));

        for (const auto& e : std::filesystem::directory_iterator(CAN, ec)) {
            if (ec)
                continue;

            if (e.is_symlink()) {
                if (!m_allowSymlink)
                    continue;
                auto target = std::filesystem::read_symlink(e, ec);
                if (!ec && std::filesystem::exists(target))
                    cacheEntry(target);
                continue;
            }

            if (e.is_regular_file() || e.is_directory())
                cacheEntry(e);
        }
    }

    auto x         = makeShared<CFsEntry>();
    x->m_path      = CAN;
    x->m_fuzzables = {CAN};
    x->m_wd        = wd;
    std::ranges::transform(x->m_fuzzables[0], x->m_fuzzables[0].begin(), ::tolower);

    m_fsEntryCache.emplace_back(x);
    m_fsEntryCacheGeneric.emplace_back(x);
    if (wd != -1)
        m_wdMap.emplace(wd, x);
    m_fsEntryCacheMap.emplace(CAN, x);
}

void CFsFinder::uncacheEntry(const std::filesystem::path& path) {
    std::error_code ec;
    const auto      CAN = std::filesystem::canonical(path, ec);
    if (ec)
        return;

    const auto it = m_fsEntryCacheMap.find(CAN);
    if (it == m_fsEntryCacheMap.end())
        return;

    const auto index = std::distance(m_fsEntryCache.begin(), std::ranges::find(m_fsEntryCache, it->second));

    m_fsEntryCache[index]        = std::move(m_fsEntryCache.back());
    m_fsEntryCacheGeneric[index] = std::move(m_fsEntryCacheGeneric.back());

    m_fsEntryCache.pop_back();
    m_fsEntryCacheGeneric.pop_back();
    if (it->second->m_wd != -1) {
        if (inotify_rm_watch(m_fd, it->second->m_wd) == -1)
            Debug::log(ERR, "fs: failed to remove watch descriptor for '{}': {}", CAN.c_str(), std::strerror(errno));
        m_wdMap.erase(it->second->m_wd);
    }
    m_fsEntryCacheMap.erase(it);
}

void CFsFinder::init() {}

void CFsFinder::loadPath() {
    if (m_fd == -1) {
        m_fd = inotify_init1(IN_NONBLOCK);
        if (m_fd == -1)
            Debug::log(ERR, "fs: failed to create file descriptor: {}", std::strerror(errno));
    }

    static auto PFSPATH    = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:fs_path");
    static auto PFSSYMLINK = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(g_configManager->m_config.get(), "finders:fs_symlink");
    m_allowSymlink         = *PFSSYMLINK;

    const std::regex env_var(R"(\$(\w+))");

    for (const auto p : std::views::split(std::string_view(*PFSPATH), ':')) {
        std::string path;
        std::size_t pos = 0;

        for (std::cregex_iterator it(p.begin(), p.end(), env_var), end; it != end; ++it) {
            const auto& match = *it;
            path.append(p.begin(), pos, match.position() - pos);
            auto v = getenv(std::string(match[1]).c_str());
            if (v)
                path.append(v);
            pos = match.position() + match.length();
        }
        path.append(path, pos, p.size() - pos);

        if (!std::filesystem::exists(path))
            continue;

        cacheEntry(path);
    }
}

std::vector<SFinderResult> CFsFinder::getResultsForQuery(const std::string& query) {
    if (m_fsEntryCache.empty())
        loadPath();
    updateEntryCache();

    auto                       fuzzed = Fuzzy::getNResults(m_fsEntryCacheGeneric, query, MAX_RESULTS_PER_FINDER, '/');

    std::vector<SFinderResult> results;
    results.reserve(fuzzed.size());

    for (const auto& f : fuzzed) {
        const auto p = reinterpretPointerCast<CFsEntry>(f);
        if (!p)
            continue;
        results.emplace_back(SFinderResult{
            .label   = p->m_path, // TODO: crop to left if gui is too smol
            .icon    = "",        // TODO: ext/dir icons
            .result  = p,
            .hasIcon = false,
        });
    }

    return results;
}
