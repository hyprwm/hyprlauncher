#include "ClientSocket.hpp"

#include <cstdlib>
#include <print>

constexpr const char*             SOCKET_NAME = ".hyprlauncher.sock";
static SP<CCHyprlauncherCoreImpl> g_coreImpl;

CClientIPCSocket::CClientIPCSocket() {
    const auto RTDIR = getenv("XDG_RUNTIME_DIR");

    if (!RTDIR)
        return;

    m_socketPath = RTDIR + std::string{"/"} + SOCKET_NAME;

    m_socket = Hyprwire::IClientSocket::open(m_socketPath);

    if (!m_socket)
        return;

    g_coreImpl = makeShared<CCHyprlauncherCoreImpl>(1);

    m_socket->addImplementation(g_coreImpl);

    if (!m_socket->waitForHandshake())
        return;

    auto spec = m_socket->getSpec(g_coreImpl->protocol()->specName());

    if (!spec) {
        m_socket.reset();
        return;
    }

    m_manager = makeShared<CCHyprlauncherCoreManagerObject>(m_socket->bindProtocol(g_coreImpl->protocol(), 1));

    if (!m_manager) {
        m_socket.reset();
        return;
    }

    m_connected = true;
}

void CClientIPCSocket::sendOpen() {
    m_manager->sendSetOpenState(1 /* open */);
}

void CClientIPCSocket::sendOpenWithOptions(const std::vector<std::string>& opts) {
    std::vector<const char*> optsC;
    optsC.reserve(opts.size());
    for (const auto& o : opts) {
        optsC.emplace_back(o.c_str());
    }

    m_info = makeShared<CCHyprlauncherCoreInfoObject>(m_manager->sendGetInfoObject());

    m_info->setOpenState([this](uint32_t open) {
        if (!open && !m_canExit) {
            m_canExit = true;
            std::println("Exited without selection");
        }
    });

    m_info->setSelectionMade([this](const char* sel) {
        std::println("{}", sel);
        m_canExit = true;
    });

    m_manager->sendOpenWithOptions(optsC);

    while (!m_canExit && m_socket->dispatchEvents(true)) {
        // wait for selection
        ;
    }
}
