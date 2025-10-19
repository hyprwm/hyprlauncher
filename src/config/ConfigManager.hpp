#pragma once
#include <hyprlang.hpp>
#include <string>

#include "../helpers/Memory.hpp"

#include <sys/inotify.h>

#include <hyprutils/os/FileDescriptor.hpp>
#include "../clipboard/ClipboardManager.hpp"

class CConfigManager {
  public:
    CConfigManager();
    void                           parse();

    void                           onInotifyEvent();

    UP<Hyprlang::CConfig>          m_config;
    Hyprutils::OS::CFileDescriptor m_inotifyFd;
    std::vector<int>               m_watches;
    std::string                    m_configPath;
    SClipboardConfig               clipboardConfig;

    void                           replantWatch();
};

inline UP<CConfigManager> g_configManager;
