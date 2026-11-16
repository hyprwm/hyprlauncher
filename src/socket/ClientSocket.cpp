#include "ClientSocket.hpp"
#include "SocketPath.hpp"

#include <algorithm>
#include <print>

static SP<CCHyprlauncherCoreImpl> g_coreImpl;

CClientIPCSocket::CClientIPCSocket(const std::string& waylandDisplay) {
    const auto socketPath = socketPathForDisplay(waylandDisplay);
    if (!socketPath)
        return;

    m_socketPath = *socketPath;

    m_socket = Hyprwire::IClientSocket::open(m_socketPath);

    if (!m_socket)
        return;

    g_coreImpl = makeShared<CCHyprlauncherCoreImpl>(2);

    m_socket->addImplementation(g_coreImpl);

    if (!m_socket->waitForHandshake())
        return;

    auto spec = m_socket->getSpec(g_coreImpl->protocol()->specName());

    if (!spec) {
        m_socket.reset();
        return;
    }

    m_protocolVersion = std::min(spec->specVer(), 2U);
    m_manager         = makeShared<CCHyprlauncherCoreManagerObject>(m_socket->bindProtocol(g_coreImpl->protocol(), m_protocolVersion));

    if (!m_manager) {
        m_socket.reset();
        return;
    }

    m_connected = true;
}

void CClientIPCSocket::sendOpen() {
    m_manager->sendSetOpenState(1 /* open */);
}

void CClientIPCSocket::sendClose() {
    m_manager->sendSetOpenState(2 /* close */);
}

void CClientIPCSocket::sendToggle() {
    m_manager->sendSetOpenState(0 /* toggle */);
}

bool CClientIPCSocket::sendOpenWithOptions(const std::vector<std::string>& opts, bool toggle) {
    if (toggle && m_protocolVersion < 2) {
        std::println(stderr, "Toggling with options requires a newer daemon. Restart the hyprlauncher daemon after updating.");
        return false;
    }

    std::vector<const char*> optsC;
    optsC.reserve(opts.size());
    for (const auto& o : opts) {
        optsC.emplace_back(o.c_str());
    }

    m_info = makeShared<CCHyprlauncherCoreInfoObject>(m_manager->sendGetInfoObject());

    m_info->setOpenState([this](uint32_t open) {
        if (!open && !m_canExit) {
            m_canExit = true;
            std::println(stderr, "Exited without selection");
        }
    });

    m_info->setSelectionMade([this](const char* sel) {
        std::println("{}", sel);
        m_canExit = true;
    });

    if (toggle)
        m_manager->sendToggleWithOptions(optsC);
    else
        m_manager->sendOpenWithOptions(optsC);

    while (!m_canExit && m_socket->dispatchEvents(true)) {
        // wait for selection
        ;
    }

    return true;
}

void CClientIPCSocket::sendSelectFinder(const std::string& finder) {
    m_manager->sendSelectFinder(finder.c_str());
}
