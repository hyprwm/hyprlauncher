#pragma once

#include "../helpers/Memory.hpp"
#include "IFinderResult.hpp"

#include <string>
#include <vector>
#include <optional>

struct SFinderResult {
    std::string                label;
    std::string                icon;
    SP<IFinderResult>          result;
    std::optional<std::string> overrideFont;
};

constexpr const size_t MAX_RESULTS_PER_FINDER = 15;

class IFinder {
  public:
    virtual ~IFinder() = default;

    virtual std::vector<SFinderResult> getResultsForQuery(const std::string& query) = 0;

    virtual void                       init();

  protected:
    IFinder() = default;
};