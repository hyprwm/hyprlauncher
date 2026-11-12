#pragma once

#include <cstdint>

enum eFinderTypes : uint8_t {
    FINDER_DESKTOP = 0,
    FINDER_UNICODE = 1,
    FINDER_MATH    = 2,
    FINDER_IPC     = 3,
    FINDER_FONT    = 4,
    FINDER_FS      = 5,
};