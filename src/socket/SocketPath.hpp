#pragma once

#include <optional>
#include <string>
#include <string_view>

std::optional<std::string> socketPathForDisplay(std::string_view waylandDisplay);
