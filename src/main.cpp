#include "ui/UI.hpp"
#include "helpers/Log.hpp"
#include "finders/desktop/DesktopFinder.hpp"
#include "finders/unicode/UnicodeFinder.hpp"
#include "finders/math/MathFinder.hpp"
#include "finders/ipc/IPCFinder.hpp"
#include "finders/font/FontFinder.hpp"
#include "socket/ClientSocket.hpp"
#include "socket/ServerSocket.hpp"
#include "query/QueryProcessor.hpp"
#include "config/ConfigManager.hpp"
#include "i18n/Engine.hpp"

#include <iostream>
#include <chrono>
#include <cstdlib>
#include <thread>

#include <hyprutils/os/ProcLock.hpp>
#include <hyprutils/string/ConstVarList.hpp>

using namespace Hyprutils::String;
using namespace Hyprutils::OS;

static void printHelp() {
    std::cout << "Hyprlauncher usage: hyprlauncher [arg [...]].\n\nArguments:\n"
              << " -d | --daemon              | Do not open after initializing\n"
              << " -o | --options \"a,b,c\"   | Pass an explicit option array\n"
              << " -m | --dmenu               | Pass an option list in dmenu-style (stdin, newline-separated)\n"
              << " -t | --toggle              | When running with this option, toggle instead of opening\n"
              << " -h | --help                | Print this menu\n"
              << " -v | --version             | Print version info\n"
              << "    | --quiet               | Disable all logging\n"
              << "    | --verbose             | Enable too much logging\n"
              << std::endl;
}

static void printVersion() {
    std::cout << "Hyprlauncher v" << HYPRLAUNCHER_VERSION << std::endl;
}

static std::vector<std::string> parseExplicitFromStdin() {
    Debug::log(TRACE, "Parsing stdin for dmenu mode");
    std::vector<std::string> result;
    std::string              line;
    while (std::getline(std::cin, line)) {
        result.emplace_back(std::move(line));
    }
    Debug::log(TRACE, "Read {} options from stdin", result.size());
    return result;
}

static const char* procLockErrorString(CProcLock::eProcLockObtainingError error) {
    switch (error) {
        case CProcLock::eProcLockObtainingError::ALREADY_TAKEN: return "lock already taken by this process";
        case CProcLock::eProcLockObtainingError::NO_ENVIRONMENT: return "XDG_RUNTIME_DIR is unset";
        case CProcLock::eProcLockObtainingError::ALREADY_RUNNING: return "another instance is running";
        case CProcLock::eProcLockObtainingError::PERMISSIONS_INSUFFICIENT: return "insufficient permissions for runtime lock";
        case CProcLock::eProcLockObtainingError::UNKNOWN: return "unknown error";
    }

    return "unknown error";
}

int main(int argc, char** argv, char** envp) {

    bool                     openByDefault = true, dmenuMode = false, toggle = false;
    std::vector<std::string> explicitOptions;

    for (int i = 1; i < argc; ++i) {
        std::string_view sv{argv[i]};

        if (sv == "--verbose") {
            Debug::verbose = true;
            continue;
        } else if (sv == "--quiet") {
            Debug::quiet = true;
            continue;
        } else if (sv == "-d" || sv == "--daemon") {
            openByDefault = false;
            continue;
        } else if (sv == "-h" || sv == "--help") {
            printHelp();
            return 0;
        } else if (sv == "-v" || sv == "--version") {
            printVersion();
            return 0;
        } else if (sv == "-m" || sv == "--dmenu") {
            dmenuMode = true;
            continue;
        } else if (sv == "-o" || sv == "--options") {
            if (i + 1 >= argc) {
                Debug::log(ERR, "Missing argument for --options", sv);
                return 1;
            }
            CConstVarList vars(argv[i + 1], 0, ',', false);
            for (const auto& e : vars) {
                explicitOptions.emplace_back(e);
            }
            ++i;
        } else if (sv == "-t" || sv == "--toggle") {
            toggle = true;
        } else {
            Debug::log(ERR, "Unrecognized argument: {}", sv);
            return 1;
        }
    }

    if (dmenuMode)
        explicitOptions = parseExplicitFromStdin();

    const auto WAYLAND_DISPLAY_ENV = getenv("WAYLAND_DISPLAY");
    const auto WAYLAND_DISPLAY     = WAYLAND_DISPLAY_ENV ? std::string{WAYLAND_DISPLAY_ENV} : std::string{};

    CProcLock  procLock{"hyprlauncher", {{"WAYLAND_DISPLAY", WAYLAND_DISPLAY}}};
    const auto lockResult = procLock.obtain(CProcLock::eProcLockFlags::EXCLUSIVE);

    if (!lockResult) {
        if (lockResult.error() != CProcLock::eProcLockObtainingError::ALREADY_RUNNING) {
            Debug::log(ERR, "Failed to obtain process lock: {}", procLockErrorString(lockResult.error()));
            return 1;
        }

        SP<CClientIPCSocket> socket = makeShared<CClientIPCSocket>(WAYLAND_DISPLAY);

        if (!socket->m_connected) {
            Debug::log(ERR, "Another instance is running, but its IPC socket is unavailable (probably starting up)");
            return 1;
        }

        Debug::log(TRACE, "Active instance already, opening launcher.");
        if (!explicitOptions.empty())
            socket->sendOpenWithOptions(explicitOptions);
        else
            toggle ? socket->sendToggle() : socket->sendOpen();
        return 0;
    }

    g_serverIPCSocket = makeUnique<CServerIPCSocket>(WAYLAND_DISPLAY);
    if (!g_serverIPCSocket->valid()) {
        Debug::log(ERR, "Failed to open IPC server socket");
        return 1;
    }

    g_desktopFinder = makeUnique<CDesktopFinder>();
    g_unicodeFinder = makeUnique<CUnicodeFinder>();
    g_mathFinder    = makeUnique<CMathFinder>();
    g_ipcFinder     = makeUnique<CIPCFinder>();
    g_fontFinder    = makeUnique<CFontFinder>();

    g_desktopFinder->init();
    g_unicodeFinder->init();
    g_mathFinder->init();
    g_ipcFinder->init();
    g_fontFinder->init();

    I18n::initEngine();

    if (!explicitOptions.empty()) {
        g_ipcFinder->setData(explicitOptions);
        g_queryProcessor->overrideQueryProvider(g_ipcFinder);
    }

    g_configManager = makeUnique<CConfigManager>();
    g_configManager->parse();

    g_ui = makeUnique<CUI>(openByDefault);
    g_ui->run();
    return 0;
}
