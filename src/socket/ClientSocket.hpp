#pragma once

#include <hyprwire/hyprwire.hpp>
#include <hyprlauncher_core-client.hpp>

#include "../helpers/Memory.hpp"

class CClientIPCSocket {
  public:
    CClientIPCSocket();
    ~CClientIPCSocket() = default;

    bool m_connected = false;

    void sendOpen();
    void sendOpenWithOptions(const std::vector<std::string>& opts);

  private:
    SP<Hyprwire::IClientSocket>         m_socket;

    SP<CCHyprlauncherCoreManagerObject> m_manager;
    SP<CCHyprlauncherCoreInfoObject>    m_info;

    bool                                m_canExit = false;

    std::string                         m_socketPath = "";
};
