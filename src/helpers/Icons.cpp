#include "Icons.hpp"

Hyprutils::Memory::CSharedPointer<Hyprtoolkit::ISystemIconDescription> applicationIcon(Hyprtoolkit::ISystemIconFactory& factory, const std::string& name,
                                                                                       const std::vector<std::string>& fallbacks, bool enabled) {
    if (!enabled)
        return {};
    if (!name.empty()) {
        auto icon = factory.lookupIcon(name);
        if (icon && icon->exists())
            return icon;
    }
    for (const auto& fallback : fallbacks) {
        auto icon = factory.lookupIcon(fallback);
        if (icon && icon->exists())
            return icon;
    }
    return {};
}

std::vector<std::string> applicationFallbackIcons(const std::string& categories, bool terminal) {
    const auto has = [&](const std::string& category) { return (";" + categories + ";").find(";" + category + ";") != std::string::npos; };
    if (terminal || has("TerminalEmulator"))
        return {"utilities-terminal", "utilities-terminal-symbolic"};
    if (has("Mixer"))
        return {"audio-volume-high", "audio-volume-high-symbolic"};
    if (has("Settings") || has("System") || has("HardwareSettings"))
        return {"preferences-system", "preferences-system-symbolic", "emblem-system-symbolic"};
    if (has("Video"))
        return {"camera-video", "camera-video-symbolic"};
    if (has("Audio"))
        return {"audio-x-generic", "audio-x-generic-symbolic"};
    return {"application-x-executable", "application-x-executable-symbolic"};
}
