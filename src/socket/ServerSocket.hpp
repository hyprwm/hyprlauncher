#pragma once

#include <hyprwire/hyprwire.hpp>
#include <hyprlauncher_core-server.hpp>

#include "../helpers/Memory.hpp"

class CServerIPCSocket {
  public:
    CServerIPCSocket();
    ~CServerIPCSocket() = default;

    void sendOpenState(bool open);
    void sendSelectionMade(const std::string& s);

  private:
    void                                            setOpenState(uint32_t state);
    void                                            openWithOptions(const std::vector<const char*>& options);

    SP<Hyprwire::IServerSocket>                     m_socket;

    std::string                                     m_socketPath = "";

    std::vector<SP<CHyprlauncherCoreManagerObject>> m_managers;
    std::vector<SP<CHyprlauncherCoreInfoObject>>    m_infos;

    friend class CUI;
};

inline UP<CServerIPCSocket> g_serverIPCSocket;
