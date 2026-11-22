#include "helpers/Icons.hpp"
#include <glib.h>
#include <cairo.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

static void check(bool condition) {
    if (!condition) {
        std::cerr << "Icon regression check failed\n";
        std::exit(1);
    }
}
int main() {
    auto* temporary = g_dir_make_tmp("hyprlauncher-icons-XXXXXX", nullptr);
    check(temporary);
    const std::filesystem::path directory(temporary);
    g_setenv("XDG_CACHE_HOME", temporary, true);
    g_free(temporary);
    const auto red  = (directory / "red.svg").string();
    const auto blue = (directory / "blue.svg").string();
    for (const auto& [path, color] : {std::pair{red, "red"}, std::pair{blue, "blue"}})
        std::ofstream(path) << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\"><rect width=\"32\" height=\"32\" fill=\"" << color << "\"/></svg>";
    check(resolveApplicationIcon(red, "hicolor", 37) == red);
    check(resolveApplicationIcon((directory / "absent.svg").string(), "hicolor", 37).empty());
    const auto redImage  = applicationIconImagePath(red, 37);
    const auto blueImage = applicationIconImagePath(blue, 37);
    check(!redImage.empty() && !blueImage.empty() && redImage != blueImage);
    for (int i = 0; i < 100; ++i) {
        check(applicationIconImagePath(red, 37) == redImage);
        check(applicationIconImagePath(blue, 37) == blueImage);
    }
    for (int size : {37, 74}) {
        const auto image   = applicationIconImagePath(red, size);
        auto*      surface = cairo_image_surface_create_from_png(image.c_str());
        check(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
        check(cairo_image_surface_get_width(surface) == size && cairo_image_surface_get_height(surface) == size);
        cairo_surface_destroy(surface);
    }
    const auto invalid = (directory / "invalid.svg").string();
    std::ofstream(invalid) << "not an image";
    check(applicationIconImagePath(invalid, 37).empty());
    // Categories are tokens, not substrings; terminal metadata takes precedence.
    check(applicationFallbackIcons("NotSettings;", false) != applicationFallbackIcons("Settings;", false));
    check(applicationFallbackIcons("Settings;", true) == applicationFallbackIcons("TerminalEmulator;", false));
    std::filesystem::remove_all(directory);
    std::cout << "Icon identity, SVG scaling, missing/invalid artwork and metadata checks passed\n";
}
