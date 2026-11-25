#pragma once
#include "../IFinderResult.hpp"
#include "../../helpers/Memory.hpp"

namespace DesktopSearch {
    std::vector<SP<IFinderResult>> match(const std::vector<SP<IFinderResult>>& entries, const std::string& query, size_t limit);
}
