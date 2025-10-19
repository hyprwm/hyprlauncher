#include "ClipboardManager.hpp"
#include <hyprutils/os/Process.hpp>
#include <memory>
#include <stdexcept>
#include "../helpers/Log.hpp"

using namespace Hyprutils::OS;

struct SPipeDeleter {
    void operator()(FILE* p) const {
        if (p)
            pclose(p);
    }
};

CClipboardManager::CClipboardManager(const SClipboardConfig& config) : m_sConfig(config) {}

void CClipboardManager::executeCommand(const std::string& command, const std::string& arg) {
    std::string finalCommand   = command;
    size_t      placeholderPos = finalCommand.find("{}");

    if (placeholderPos != std::string::npos) {
        finalCommand.replace(placeholderPos, 2, arg);
        try {
            CProcess proc("/bin/sh", {"-c", finalCommand});
            proc.runAsync();
        } catch (const std::exception& e) { Debug::log(ERR, "Failed to execute command: {}", e.what()); }
    } else {
        std::unique_ptr<FILE, SPipeDeleter> pipe(popen(command.c_str(), "w"));
        if (!pipe) {
            Debug::log(ERR, "popen() failed for command {}", command);
            return;
        }
        if (!arg.empty()) {
            if (fwrite(arg.c_str(), sizeof(char), arg.length(), pipe.get()) != arg.length()) {
                Debug::log(ERR, "Warning: Incomplete write to pipe for command: ", command);
            }
        }
    }
}

std::vector<std::string> CClipboardManager::getHistory() {
    std::unique_ptr<FILE, SPipeDeleter> pipe(popen(m_sConfig.list_cmd.c_str(), "r"));
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    std::vector<std::string> history;
    char                     buf[4096];

    while (fgets(buf, sizeof(buf), pipe.get())) {
        std::string s(buf);
        if (!s.empty() && s.back() == '\n') {
            s.pop_back();
        }
        history.push_back(s);
    }
    return history;
}

void CClipboardManager::copyItem(const std::string& item) {
    executeCommand(m_sConfig.copy_cmd, item);
}

void CClipboardManager::deleteItem(const std::string& item) {
    executeCommand(m_sConfig.delete_cmd, item);
}

void CClipboardManager::paste() {
    executeCommand(m_sConfig.paste_cmd, "");
}

SClipboardConfig CClipboardManager::getConfig() const {
    return m_sConfig;
}
