#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Return a local image path; an empty result requests the built-in fallback.
std::string              resolveApplicationIcon(const std::string& name, const std::string& theme, int pixelSize);

std::vector<uint8_t>     loadApplicationIcon(const std::string& path, int pixelSize);

std::string              applicationIconImagePath(const std::string& path, int pixelSize);

std::vector<std::string> applicationFallbackIcons(const std::string& categories, bool terminal);
