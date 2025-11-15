#include "Engine.hpp"
#include "../config/ConfigManager.hpp"

static Hyprutils::I18n::CI18nEngine engine;

//
void I18n::initEngine() {
    engine.setFallbackLocale("en_US");

    // en_US (English)
    engine.registerEntry("en_US", TXT_KEY_SEARCH_SOMETHING, "Search...");

    // ru_RU (Russian)
    engine.registerEntry("ru_RU", TXT_KEY_SEARCH_SOMETHING, "Поиск...");

    // pl_PL (Polish)
    engine.registerEntry("pl_PL", TXT_KEY_SEARCH_SOMETHING, "Szukaj...");

    // ja_JP (Japanese)
    engine.registerEntry("ja_JP", TXT_KEY_SEARCH_SOMETHING, "検索...");

    // zh (Simplified Chinese)
    engine.registerEntry("zh", TXT_KEY_SEARCH_SOMETHING, "搜索...");

    // zh_Hant (Traditional Chinese)
    engine.registerEntry("zh_Hant", TXT_KEY_SEARCH_SOMETHING, "搜尋...");
}

std::string I18n::localize(eTextKeys key, const Hyprutils::I18n::translationVarMap& vars) {
    static auto POVERRIDELOCALE = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "locale:override");

    const auto  LOCALE = std::string_view{*POVERRIDELOCALE}.empty() ? engine.getSystemLocale().locale() : std::string{*POVERRIDELOCALE};

    return engine.localizeEntry(LOCALE, key, vars);
}
