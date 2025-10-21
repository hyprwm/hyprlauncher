#include "ClipboardManager.hpp"
#include <hyprutils/os/Process.hpp>
#include <hyprutils/string/String.hpp>
#include <iterator>
#include "../helpers/Log.hpp"
#include <ranges>
#include <sstream>

using namespace Hyprutils::OS;

struct SPipeDeleter {
    void operator()(FILE* p) const {
        if (p)
            pclose(p);
    }
};

CClipboardManager::CClipboardManager(const SClipboardConfig& config) : m_sConfig(config) {}

static void executeCommand(const std::string& command, const std::string& arg) {
    if (command.empty())
        return;

    std::string finalCommand   = command;
    size_t      placeholderPos = finalCommand.find("{}");

    if (placeholderPos != std::string::npos) {
        finalCommand.replace(placeholderPos, 2, "\"$1\"");
        CProcess proc("/bin/sh", {"-c", finalCommand, "-", arg});
        proc.runAsync();
    } else if (!arg.empty()) {
        std::string shellCommand = "echo -n \"$1\" | " + command;
        CProcess proc("/bin/sh", {"-c", shellCommand, "-", arg});
        proc.runAsync();
    } else {
        CProcess proc("/bin/sh", {"-c", command});
        proc.runAsync();
    }
}

std::vector<SClipboardHistoryItem> CClipboardManager::getHistory() {
    std::istringstream iss(m_sConfig.list_cmd);
    std::vector<std::string> parts{
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    };

    if (parts.empty()) {
        Debug::log(ERR, "Empty list_cmd");
        return {};
    }

    const std::string& binary = parts.front();
    std::vector<std::string> args(parts.begin() + 1, parts.end());
    CProcess proc(binary, args);
    proc.runSync();
    const std::string output = proc.stdOut();
    const std::string errOut = proc.stdErr();

    if (!errOut.empty())
        Debug::log(WARN, "list_cmd stderr: {}", errOut);

    const int exitCode = proc.exitCode();
    if (exitCode != 0) {
        Debug::log(ERR, "list_cmd exited with code {}: {}", exitCode, m_sConfig.list_cmd);
        return {};
    }
    std::vector<SClipboardHistoryItem> history;
    for (const auto && line_view : std::ranges::views::split(output, '\n')) {
        std::string line{line_view.begin(), line_view.end()};
        if (line.empty()) continue;

        const auto tabPos = line.find('\t');
        std::string_view display = (tabPos != std::string::npos)
            ? std::string_view(line).substr(tabPos + 1)
            : std::string_view(line);
        history.push_back({line, std::string(display)});
    }
    return history;
}

void CClipboardManager::copyItem(const std::string& original_line) {
    executeCommand(m_sConfig.copy_cmd, original_line);
}

void CClipboardManager::deleteItem(const std::string& item) {
    executeCommand(m_sConfig.delete_cmd, item);
}

SClipboardConfig CClipboardManager::getConfig() const {
    return m_sConfig;
}
