#pragma once
#include <hyprlang.hpp>
#include <string>

#include "../helpers/Memory.hpp"

#include <sys/inotify.h>

#include <hyprutils/os/FileDescriptor.hpp>

class CConfigManager {
  public:
    CConfigManager(const std::string& configPath);

    void                           parse();

    void                           onInotifyEvent();

    UP<Hyprlang::CConfig>          m_config;
    Hyprutils::OS::CFileDescriptor m_inotifyFd;
    std::vector<int>               m_watches;
    std::string                    m_configPath;

    void                           replantWatch();
};

inline UP<CConfigManager> g_configManager;
