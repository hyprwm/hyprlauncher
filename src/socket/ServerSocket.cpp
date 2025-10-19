#include "ServerSocket.hpp"
#include "../ui/UI.hpp"
#include "../query/QueryProcessor.hpp"
#include "../finders/ipc/IPCFinder.hpp"

#include <cstdlib>
#include <filesystem>

constexpr const char*            SOCKET_NAME = ".hyprlauncher.sock";

static SP<CHyprlauncherCoreImpl> g_coreImpl;

CServerIPCSocket::CServerIPCSocket() {
    const auto RTDIR = getenv("XDG_RUNTIME_DIR");

    if (!RTDIR)
        return;

    m_socketPath = RTDIR + std::string{"/"} + SOCKET_NAME;

    std::error_code ec;
    std::filesystem::remove(m_socketPath, ec);

    m_socket = Hyprwire::IServerSocket::open(m_socketPath);

    if (!m_socket)
        return;

    g_coreImpl = makeShared<CHyprlauncherCoreImpl>(1, [this](SP<Hyprwire::IObject> obj) {
        auto manager = m_managers.emplace_back(makeShared<CHyprlauncherCoreManagerObject>(std::move(obj)));

        manager->setSetOpenState([this](uint32_t state) { setOpenState(state); });
        manager->setOpenWithOptions([this](std::vector<const char*> state) { openWithOptions(state); });

        manager->setGetInfoObject([this, m = WP<CHyprlauncherCoreManagerObject>{manager}](uint32_t seq) {
            if (!m)
                return; // protocol error

            auto x = m_infos.emplace_back(makeShared<CHyprlauncherCoreInfoObject>(m_socket->createObject(m->getObject()->client(), m->getObject(), "hyprlauncher_core_info", seq)));

            x->setOnDestroy([this, weak = WP<CHyprlauncherCoreInfoObject>{x}] { std::erase(m_infos, weak); });
        });

        manager->setOnDestroy([this, weak = WP<CHyprlauncherCoreManagerObject>{manager}] { std::erase(m_managers, weak); });
    });

    m_socket->addImplementation(g_coreImpl);
}

void CServerIPCSocket::setOpenState(uint32_t state) {
    switch (state) {
        case 0: g_ui->setWindowOpen(!g_ui->windowOpen()); break;
        case 1: g_ui->setWindowOpen(true); break;
        case 2: g_ui->setWindowOpen(false); break;
        default: break;
    }
}

void CServerIPCSocket::openWithOptions(const std::vector<const char*>& options) {
    if (g_ui->windowOpen())
        return;

    g_ipcFinder->setData(options);
    g_queryProcessor->overrideQueryProvider(g_ipcFinder);
    g_ui->setWindowOpen(true);
}

void CServerIPCSocket::sendOpenState(bool open) {
    for (const auto& i : m_infos) {
        i->sendOpenState(open);
    }
}

void CServerIPCSocket::sendSelectionMade(const std::string& s) {
    for (const auto& i : m_infos) {
        i->sendSelectionMade(s.c_str());
    }
}
