#include "ui/UI.hpp"
#include "helpers/Log.hpp"
#include "finders/desktop/DesktopFinder.hpp"
#include "finders/unicode/UnicodeFinder.hpp"
#include "finders/math/MathFinder.hpp"
#include "finders/clipboard/ClipboardFinder.hpp"
#include "finders/ipc/IPCFinder.hpp"
#include "socket/ClientSocket.hpp"
#include "socket/ServerSocket.hpp"
#include "query/QueryProcessor.hpp"
#include "config/ConfigManager.hpp"

#include <algorithm>
#include <csignal>
#include <hyprutils/string/ConstVarList.hpp>
#include <iostream>
#include <string_view>
#include <vector>

using namespace Hyprutils::String;

static void printHelp() {
    std::cout << "Hyprlauncher usage: hyprlauncher [arg [...]].\n\nArguments:\n"
              << " -d | --daemon              | Do not open after initializing\n"
              << " -o | --options \"a,b,c\"   | Pass an explicit option array\n"
              << " -p | --provider <name>     | Launch with a specific provider (e.g., clipboard)\n"
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
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    bool                     openByDefault = true;
    std::vector<std::string> explicitOptions;
    std::string              provider;

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
        } else if (sv == "-p" || sv == "--provider") {
            if (i + 1 >= argc) {
                Debug::log(ERR, "Missing argument for --provider");
                return 1;
            }
            provider = argv[i + 1];
            ++i;
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
        }
    }

    auto socket = makeShared<CClientIPCSocket>();

    if (socket->m_connected) {
        Debug::log(TRACE, "Active instance already, opening launcher.");
        if (!provider.empty() || !explicitOptions.empty())
            socket->sendOpenWithProvider(provider.empty() ? "ipc" : provider, explicitOptions);
        else
            socket->sendOpen();
        return 0;
    }

    g_serverIPCSocket = makeUnique<CServerIPCSocket>();

    g_desktopFinder = makeUnique<CDesktopFinder>();
    g_unicodeFinder = makeUnique<CUnicodeFinder>();
    g_mathFinder    = makeUnique<CMathFinder>();
    g_ipcFinder     = makeUnique<CIPCFinder>();
    g_clipboardFinder = makeUnique<CClipboardFinder>();

    g_desktopFinder->init();
    g_unicodeFinder->init();
    g_mathFinder->init();
    g_ipcFinder->init();
    g_clipboardFinder->init();

    g_configManager = makeUnique<CConfigManager>();
    g_configManager->parse();

    g_clipboardFinder->onConfigReload();

    socket.reset();

    if (!explicitOptions.empty()) {
        auto it = std::ranges::find_if(explicitOptions, [](const std::string& s){
            return s.ends_with("_mode");
        });

        if (it != explicitOptions.end()) {
            g_queryProcessor->setProviderByName(it->substr(0, it->size() - 5));
            explicitOptions.erase(it);
        } else {
            g_ipcFinder->setData(explicitOptions);
            g_queryProcessor->overrideQueryProvider(g_ipcFinder);
        }
    } else if (!provider.empty()) {
        if (!g_queryProcessor->setProviderByName(provider))
            Debug::log(WARN, "Unknown provider '{}'. Using default.", provider);
    }

    g_ui = makeUnique<CUI>(openByDefault);
    g_ui->run();
    return 0;
}
