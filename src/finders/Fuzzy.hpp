#pragma once

#include <initializer_list>
#include <string>
#include <vector>

#include "IFinderResult.hpp"
#include "../helpers/Memory.hpp"

namespace Fuzzy {
    std::vector<SP<IFinderResult>> getNResults(const std::vector<SP<IFinderResult>>& in, const std::string& query, size_t results);
    std::vector<std::string>       createFuzzableStrings(std::initializer_list<std::string_view>, bool toLowercase = true);
};
