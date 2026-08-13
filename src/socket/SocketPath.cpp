#include "SocketPath.hpp"

#include <cstdint>
#include <cstdlib>
#include <format>

std::optional<std::string> socketPathForDisplay(std::string_view waylandDisplay) {
    const auto runtimeDir = getenv("XDG_RUNTIME_DIR");
    if (!runtimeDir || runtimeDir[0] == '\0')
        return std::nullopt;

    uint64_t displayHash = 14695981039346656037ULL;
    for (const unsigned char c : waylandDisplay) {
        displayHash ^= c;
        displayHash *= 1099511628211ULL;
    }

    return std::format("{}/.hyprlauncher-{:016x}.sock", runtimeDir, displayHash);
}
