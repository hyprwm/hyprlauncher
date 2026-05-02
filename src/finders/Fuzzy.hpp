#pragma once

#include <string>
#include <vector>

#include "IFinderResult.hpp"
#include "../helpers/Memory.hpp"

namespace Fuzzy {
    std::vector<SP<IFinderResult>> getNResults(const std::vector<SP<IFinderResult>>& in, const std::string& query, size_t results, char tokenBreak = ' ');
};