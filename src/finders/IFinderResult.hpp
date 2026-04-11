#pragma once

#include <string>
#include <vector>
#include "FinderTypes.hpp"

class IFinderResult {
  public:
    virtual ~IFinderResult() = default;

    virtual eFinderTypes                    type()      = 0;
    virtual void                            run()       = 0;
    virtual const std::vector<std::string>& fuzzables() = 0;
    virtual const std::string&              name()      = 0;
    virtual uint32_t                        frequency();

  protected:
    IFinderResult() = default;
};
