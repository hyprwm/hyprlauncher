#pragma once

#include <cstdint>
#include <string>
#include <hyprutils/i18n/I18nEngine.hpp>

namespace I18n {
    //NOLINTNEXTLINE
    enum eTextKeys : uint64_t {
        TXT_KEY_SEARCH_SOMETHING = 0,
    };

    void        initEngine();
    std::string localize(eTextKeys key, const Hyprutils::I18n::translationVarMap& vars);
};
