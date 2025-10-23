#include "ClipboardManager.hpp"
#include <hyprutils/os/Process.hpp>
#include <iterator>
#include "../helpers/Log.hpp"
#include <ranges>
#include <sstream>

using namespace Hyprutils::OS;

CClipboardManager::CClipboardManager(const SClipboardConfig& config) : m_Config(config) {}

static void executeCommand(const std::string& command, const std::string& arg) {
    if (command.empty())
        return;

    bool hasPlaceholder = command.find('{') != std::string::npos;

    if (hasPlaceholder) {
        std::string finalCommand;
        try {
            finalCommand = std::vformat(command, std::make_format_args("\"$1\""));
        } catch (const std::format_error& e) {
            Debug::log(ERR, "Invalid format string in command: {}. Error: {}", command, e.what());
            return;
        }
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
    std::istringstream iss(m_Config.listCmd);
    std::vector<std::string> parts;
    std::string part;
    while (iss >> part) {
        parts.push_back(part);
    }

    if (parts.empty()) {
        Debug::log(ERR, "Empty listCmd");
        return {};
    }

    const std::string& binary = parts.front();
    std::vector<std::string> args(parts.begin() + 1, parts.end());
    CProcess proc(binary, args);
    proc.runSync();
    const std::string output = proc.stdOut();
    const std::string errOut = proc.stdErr();

    if (!errOut.empty())
        Debug::log(WARN, "listCmd stderr: {}", errOut);

    const int exitCode = proc.exitCode();
    if (exitCode != 0) {
        Debug::log(ERR, "listCmd exited with code {}: {}", exitCode, m_Config.listCmd);
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

void CClipboardManager::copyItem(const std::string& originalLine) {
    executeCommand(m_Config.copyCmd, originalLine);
}

void CClipboardManager::deleteItem(const std::string& item) {
    executeCommand(m_Config.deleteCmd, item);
}

SClipboardConfig CClipboardManager::getConfig() const {
    return m_Config;
}
