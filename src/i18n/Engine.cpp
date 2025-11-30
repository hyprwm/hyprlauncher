#include "Engine.hpp"
#include "../config/ConfigManager.hpp"

static Hyprutils::I18n::CI18nEngine engine;

//
void I18n::initEngine() {
    engine.setFallbackLocale("en_US");

    // ar (Arabic)
    engine.registerEntry("ar", TXT_KEY_SEARCH_SOMETHING, "بحث...");

    // as_IN (Assamese)
    engine.registerEntry("as_IN", TXT_KEY_SEARCH_SOMETHING, "অনুসন্ধান...");

    // ca_ES (Catalan)
    engine.registerEntry("ca_ES", TXT_KEY_SEARCH_SOMETHING, "Cercar...");

    // cs_CZ (Czech)
    engine.registerEntry("cs_CZ", TXT_KEY_SEARCH_SOMETHING, "Hledat...");

    // da_DK (Danish)
    engine.registerEntry("da_DK", TXT_KEY_SEARCH_SOMETHING, "Søg...");

    // de_DE (German)
    engine.registerEntry("de_DE", TXT_KEY_SEARCH_SOMETHING, "Suche...");

    // el_GR (Greek)
    engine.registerEntry("el_GR", TXT_KEY_SEARCH_SOMETHING, "Αναζήτηστε...");

    // en_US (English)
    engine.registerEntry("en_US", TXT_KEY_SEARCH_SOMETHING, "Search...");

    // es (Spanish)
    engine.registerEntry("es", TXT_KEY_SEARCH_SOMETHING, "Buscar...");

    // fa (Farsi/Persian)
    engine.registerEntry("fa", TXT_KEY_SEARCH_SOMETHING, "جستجو...");

    // fi_FI (Finnish)
    engine.registerEntry("fi_FI", TXT_KEY_SEARCH_SOMETHING, "Hae...");

    // fr_FR (French)
    engine.registerEntry("fr_FR", TXT_KEY_SEARCH_SOMETHING, "Rechercher...");

    // he (Hebrew)
    engine.registerEntry("he", TXT_KEY_SEARCH_SOMETHING, "חיפוש...");

    // hi_IN (Hindi)
    engine.registerEntry("hi_IN", TXT_KEY_SEARCH_SOMETHING, "खोजें...");

    // hu_HU (Hungarian)
    engine.registerEntry("hu_HU", TXT_KEY_SEARCH_SOMETHING, "Keresés...");

    // id_ID (Indonesian)
    engine.registerEntry("id_ID", TXT_KEY_SEARCH_SOMETHING, "Cari...");

    // it_IT (Italian)
    engine.registerEntry("it_IT", TXT_KEY_SEARCH_SOMETHING, "Cerca...");

    // ja_JP (Japanese)
    engine.registerEntry("ja_JP", TXT_KEY_SEARCH_SOMETHING, "検索...");

    // ku (Kurdish)
    engine.registerEntry("ku", TXT_KEY_SEARCH_SOMETHING, "Bigere...");

    // lt_LT (Lithuanian)
    engine.registerEntry("lt_LT", TXT_KEY_SEARCH_SOMETHING, "Paieška...");

    // ml_IN (Malayalam)
    engine.registerEntry("ml_IN", TXT_KEY_SEARCH_SOMETHING, "തിരയുക...");

    // nb_NO (Norwegian Bokmål)
    engine.registerEntry("nb_NO", TXT_KEY_SEARCH_SOMETHING, "Søk...");

    // nl_NL (Dutch Netherlands)
    engine.registerEntry("nl_NL", TXT_KEY_SEARCH_SOMETHING, "Zoeken...");

    // pl_PL (Polish)
    engine.registerEntry("pl_PL", TXT_KEY_SEARCH_SOMETHING, "Szukaj...");

    // pt_BR (Portuguese BR)
    engine.registerEntry("pt_BR", TXT_KEY_SEARCH_SOMETHING, "Buscar...");

    // pt_PT (Portuguese Portugal)
    engine.registerEntry("pt_PT", TXT_KEY_SEARCH_SOMETHING, "Procurar...");

    // pa_IN (Punjabi)
    engine.registerEntry("pa_IN", TXT_KEY_SEARCH_SOMETHING, "ਖੋਜ...");

    // ro_RO (Romanian)
    engine.registerEntry("ro_RO", TXT_KEY_SEARCH_SOMETHING, "Caută...");

    // ru_RU (Russian)
    engine.registerEntry("ru_RU", TXT_KEY_SEARCH_SOMETHING, "Поиск...");

    // sl_SI (Slovenian)
    engine.registerEntry("sl_SI", TXT_KEY_SEARCH_SOMETHING, "Iščite...");

    // sr_RS (Serbian)
    engine.registerEntry("sr_RS", TXT_KEY_SEARCH_SOMETHING, "Тражи...");

    // sr_RS@latin (Serbian Latin)
    engine.registerEntry("sr_RS@latin", TXT_KEY_SEARCH_SOMETHING, "Traži...");

    // sv_SE (Swedish)
    engine.registerEntry("sv_SE", TXT_KEY_SEARCH_SOMETHING, "Sök...");

    // ta_IN (Tamil)
    engine.registerEntry("ta_IN", TXT_KEY_SEARCH_SOMETHING, "தேடவும்...");

    // te_IN (Telugu)
    engine.registerEntry("te_IN", TXT_KEY_SEARCH_SOMETHING, "శోధన...");

    // tr_TR (Turkish)
    engine.registerEntry("tr_TR", TXT_KEY_SEARCH_SOMETHING, "Ara...");

    // tt_RU (Tatar)
    engine.registerEntry("tt_RU", TXT_KEY_SEARCH_SOMETHING, "Эзләү...");

    // uk_UA (Ukrainian)
    engine.registerEntry("uk_UA", TXT_KEY_SEARCH_SOMETHING, "Пошук...");

    // vi_VN (Vietnamese)
    engine.registerEntry("vi_VN", TXT_KEY_SEARCH_SOMETHING, "Tìm kiếm...");

    // zh (Simplified Chinese)
    engine.registerEntry("zh", TXT_KEY_SEARCH_SOMETHING, "搜索...");

    // zh_Hant (Traditional Chinese)
    engine.registerEntry("zh_Hant", TXT_KEY_SEARCH_SOMETHING, "搜尋...");

    // en_NP (Romanized Nepali)
    engine.registerEntry("en_NP", TXT_KEY_SEARCH_SOMETHING, "Khoja...");

    // ne_NP (Nepali)
    engine.registerEntry("ne_NP", TXT_KEY_SEARCH_SOMETHING, "खोज...");
}

std::string I18n::localize(eTextKeys key, const Hyprutils::I18n::translationVarMap& vars) {
    static auto POVERRIDELOCALE = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "locale:override");

    const auto  LOCALE = std::string_view{*POVERRIDELOCALE}.empty() ? engine.getSystemLocale().locale() : std::string{*POVERRIDELOCALE};

    return engine.localizeEntry(LOCALE, key, vars);
}
