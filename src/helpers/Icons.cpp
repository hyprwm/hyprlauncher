#include "Icons.hpp"

#include <gtk/gtk.h>
#include <hyprgraphics/resource/resources/ImageResource.hpp>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <map>

std::string resolveApplicationIcon(const std::string& name, const std::string& theme, int pixelSize) {
    if (name.empty())
        return {};

    if (std::filesystem::path(name).is_absolute()) {
        std::error_code ec;
        return std::filesystem::is_regular_file(name, ec) && !ec ? name : std::string{};
    }

    // GTK implements theme inheritance and selects artwork for the requested
    // physical size, rather than taking the first matching raster image.
    static const auto icons = std::unique_ptr<GtkIconTheme, decltype(&g_object_unref)>(gtk_icon_theme_new(), g_object_unref);
    gtk_icon_theme_set_theme_name(icons.get(), theme.empty() ? "hicolor" : theme.c_str());
    if (!gtk_icon_theme_has_icon(icons.get(), name.c_str()))
        return {};

    auto* paintable = gtk_icon_theme_lookup_icon(icons.get(), name.c_str(), nullptr, std::max(1, pixelSize), 1, GTK_TEXT_DIR_NONE, static_cast<GtkIconLookupFlags>(0));
    if (!paintable)
        return {};
    auto*       file   = gtk_icon_paintable_get_file(paintable);
    auto*       path   = file ? g_file_get_path(file) : nullptr;
    std::string result = path ? path : "";
    g_free(path);
    g_object_unref(paintable);
    return result;
}

std::vector<uint8_t> loadApplicationIcon(const std::string& path, int pixelSize) {
    if (path.empty())
        return {};
    pixelSize = std::max(1, pixelSize);
    using Key = std::pair<std::string, int>;
    static std::map<Key, std::vector<uint8_t>> cache;
    const Key                                  key{path, pixelSize};
    if (const auto found = cache.find(key); found != cache.end())
        return found->second;

    // The toolkit's path loader does not give SVGs a rasterization size.
    // Decode at the physical display size, preserving the source aspect ratio.
    Hyprgraphics::CImageResource resource(path, {double(pixelSize), double(pixelSize)});
    resource.render();
    std::vector<uint8_t> bytes;
    if (resource.m_asset.cairoSurface) {
        const auto status = cairo_surface_write_to_png_stream(
            resource.m_asset.cairoSurface->cairo(),
            [](void* target, const unsigned char* data, unsigned int size) {
                auto& output = *static_cast<std::vector<uint8_t>*>(target);
                output.insert(output.end(), data, data + size);
                return CAIRO_STATUS_SUCCESS;
            },
            &bytes);
        if (status != CAIRO_STATUS_SUCCESS)
            bytes.clear();
    }
    // Bound memory usage when changing sizes or browsing many applications.
    if (cache.size() >= 256)
        cache.clear();
    cache.emplace(key, bytes);
    return bytes;
}

std::string applicationIconImagePath(const std::string& path, int pixelSize) {
    const auto bytes = loadApplicationIcon(path, pixelSize);
    if (bytes.empty())
        return {};

    // Hyprtoolkit keys in-memory images by their buffer address. Reused row
    // buffers can therefore retrieve a different app's old texture. Give each
    // rendered image a stable, content-addressed PNG path instead.
    auto*      checksum  = g_compute_checksum_for_data(G_CHECKSUM_SHA256, bytes.data(), bytes.size());
    const auto directory = std::filesystem::path(g_get_user_cache_dir()) / "hyprlauncher" / "icons";
    const auto image     = directory / (std::string(checksum) + ".png");
    g_free(checksum);
    if (g_mkdir_with_parents(directory.c_str(), 0700) != 0)
        return {};
    std::error_code ec;
    if (std::filesystem::is_regular_file(image, ec) && !ec)
        return image.string();
    GError*    error = nullptr;
    const bool saved = g_file_set_contents(image.c_str(), reinterpret_cast<const char*>(bytes.data()), bytes.size(), &error);
    if (error)
        g_error_free(error);
    return saved ? image.string() : std::string{};
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
