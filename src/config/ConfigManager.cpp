#include "ConfigManager.hpp"

#include "../helpers/Log.hpp"

#include <unistd.h>
#include <climits>

#include <hyprutils/path/Path.hpp>

CConfigManager::CConfigManager() : m_inotifyFd(inotify_init()) {
    const auto CFGPATH = Hyprutils::Path::findConfig("hyprlauncher").first.value_or("");
    m_configPath       = CFGPATH;

    m_config = makeUnique<Hyprlang::CConfig>(CFGPATH.c_str(), Hyprlang::SConfigOptions{.allowMissingConfig = true});

    m_config->addConfigValue("general:grab_focus", Hyprlang::INT{1});

    m_config->addConfigValue("locale:override", Hyprlang::STRING{""});

    m_config->addConfigValue("cache:enabled", Hyprlang::INT{1});

    m_config->addConfigValue("finders:default_finder", Hyprlang::STRING{"desktop"});
    m_config->addConfigValue("finders:desktop_prefix", Hyprlang::STRING{""});
    m_config->addConfigValue("finders:unicode_prefix", Hyprlang::STRING{"."});
    m_config->addConfigValue("finders:math_prefix", Hyprlang::STRING{"="});
    m_config->addConfigValue("finders:font_prefix", Hyprlang::STRING{"'"});
    m_config->addConfigValue("finders:fs_prefix", Hyprlang::STRING{"/"});

    m_config->addConfigValue("finders:desktop_launch_prefix", Hyprlang::STRING{""});
    m_config->addConfigValue("finders:desktop_icons", Hyprlang::INT{1});

    m_config->addConfigValue("ui:window_size", Hyprlang::VEC2{400, 260});

    m_config->commence();

    replantWatch();
}

void CConfigManager::replantWatch() {
    for (const auto& w : m_watches) {
        inotify_rm_watch(m_inotifyFd.get(), w);
    }

    m_watches.clear();

    m_watches.emplace_back(inotify_add_watch(m_inotifyFd.get(), m_configPath.c_str(), IN_MODIFY | IN_DONT_FOLLOW));
}

void CConfigManager::parse() {
    const auto ERROR = m_config->parse();

    if (ERROR.error)
        Debug::log(ERR, "Error in config: {}", ERROR.getError());
}

void CConfigManager::onInotifyEvent() {
    constexpr size_t                                     BUFFER_SIZE = sizeof(inotify_event) + NAME_MAX + 1;
    alignas(inotify_event) std::array<char, BUFFER_SIZE> buffer      = {};
    const ssize_t                                        bytesRead   = read(m_inotifyFd.get(), buffer.data(), buffer.size());
    if (bytesRead <= 0)
        return;

    for (size_t offset = 0; offset < sc<size_t>(bytesRead);) {
        const auto* ev = rc<const inotify_event*>(buffer.data() + offset);

        if (offset + sizeof(inotify_event) > sc<size_t>(bytesRead)) {
            // err
            break;
        }

        if (offset + sizeof(inotify_event) + ev->len > sc<size_t>(bytesRead)) {
            // err
            break;
        }

        offset += sizeof(inotify_event) + ev->len;
    }

    replantWatch();

    parse();
}
