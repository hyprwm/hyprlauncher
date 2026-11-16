#pragma once

#include <hyprwire/hyprwire.hpp>
#include <hyprlauncher_core-client.hpp>

#include "../helpers/Memory.hpp"

class CClientIPCSocket {
  public:
    CClientIPCSocket(const std::string& waylandDisplay);
    ~CClientIPCSocket() = default;

    bool m_connected = false;

    void sendOpen();
    void sendClose();
    void sendToggle();
    bool sendOpenWithOptions(const std::vector<std::string>& opts, bool toggle);
    void sendSelectFinder(const std::string& finder);

  private:
    SP<Hyprwire::IClientSocket>         m_socket;

    SP<CCHyprlauncherCoreManagerObject> m_manager;
    SP<CCHyprlauncherCoreInfoObject>    m_info;

    bool                                m_canExit         = false;
    uint32_t                            m_protocolVersion = 1;

    std::string                         m_socketPath = "";
};
