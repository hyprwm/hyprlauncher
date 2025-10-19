#include "finders/clipboard/ClipboardFinder.hpp"
#include "ui/UI.hpp"
#include "helpers/Log.hpp"
#include "finders/desktop/DesktopFinder.hpp"
#include "finders/unicode/UnicodeFinder.hpp"
#include "finders/math/MathFinder.hpp"
#include "finders/ipc/IPCFinder.hpp"
#include "socket/ClientSocket.hpp"
#include "socket/ServerSocket.hpp"
#include "query/QueryProcessor.hpp"
#include "config/ConfigManager.hpp"

#include <iostream>

#include <hyprutils/string/ConstVarList.hpp>

using namespace Hyprutils::String;

static void printHelp() {
    std::cout << "Hyprlauncher usage: hyprlauncher [arg [...]].\n\nArguments:\n"
              << " -d | --daemon              | Do not open after initializing\n"
              << " -o | --options \"a,b,c\"   | Pass an explicit option array\n"
              << " -h | --help                | Print this menu\n"
              << " -v | --version             | Print version info\n"
              << "    | --quiet               | Disable all logging\n"
              << "    | --verbose             | Enable too much logging\n"
              << std::endl;
}

static void printVersion() {
    std::cout << "Hyprlauncher v" << HYPRLAUNCHER_VERSION << std::endl;
}

int main(int argc, char** argv, char** envp) {

    bool                     openByDefault = true;
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
        } else {
            Debug::log(ERR, "Unrecognized argument: {}", sv);
            return 1;
        }
    }

    auto socket = makeShared<CClientIPCSocket>();

    if (socket->m_connected) {
        Debug::log(TRACE, "Active instance already, opening launcher.");
        if (!explicitOptions.empty())
            socket->sendOpenWithOptions(explicitOptions);
        else
            socket->sendOpen();
        return 0;
    }

    g_serverIPCSocket = makeUnique<CServerIPCSocket>();

    g_desktopFinder = makeUnique<CDesktopFinder>();
    g_unicodeFinder = makeUnique<CUnicodeFinder>();
    g_mathFinder    = makeUnique<CMathFinder>();
    g_ipcFinder     = makeUnique<CIPCFinder>();

    g_desktopFinder->init();
    g_unicodeFinder->init();
    g_mathFinder->init();
    g_ipcFinder->init();

    socket.reset();

    if (!explicitOptions.empty()) {
        g_ipcFinder->setData(explicitOptions);
        g_queryProcessor->overrideQueryProvider(g_ipcFinder);
    }

    g_configManager = makeUnique<CConfigManager>();
    g_configManager->parse();

    g_clipboardFinder = makeUnique<CClipboardFinder>();
    g_clipboardFinder->init();
    g_clipboardFinder->onConfigReload();

    g_ui = makeUnique<CUI>(openByDefault);
    g_ui->run();
    return 0;
}
