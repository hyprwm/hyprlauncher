#include "Cache.hpp"
#include "../config/ConfigManager.hpp"

#include <filesystem>
#include <fstream>
#include <chrono>

#include <hyprutils/string/String.hpp>
#include <hyprutils/string/ConstVarList.hpp>
#include <hyprutils/memory/Casts.hpp>

using namespace Hyprutils::String;
using namespace Hyprutils::Memory;

constexpr const size_t CACHE_MAX_SIZE = 512;

CEntryCache::CEntryCache(const std::string& name) {
    const auto HOME = getenv("HOME");

    if (!HOME)
        return;

    std::error_code ec;
    if (!std::filesystem::exists(HOME + std::string{"/.local/share/hyprlauncher"}, ec) || ec)
        std::filesystem::create_directory(HOME + std::string{"/.local/share/hyprlauncher"});
    m_cacheFullPath = HOME + std::string{"/.local/share/hyprlauncher/" + name + ".cache"};

    // load cache
    std::ifstream ifs(m_cacheFullPath);

    if (!ifs.good())
        return;

    std::string line;
    while (std::getline(ifs, line)) {
        m_cache[line].value += 1;
        m_cacheStrings.emplace_back(line);
    }

    m_good = true;
}

bool CEntryCache::good() {
    return m_good;
}

uint32_t CEntryCache::getCachedEntry(const std::string& entry) {
    try {
        return m_cache.at(entry).value;
    } catch (...) { ; }

    return 0;
}

void CEntryCache::incrementCachedEntry(const std::string& entry) {
    auto& e = m_cache[entry];
    ++e.value;

    m_cacheStrings.emplace_back(entry);

    save();
}

void CEntryCache::trimCache() {
    if (m_cacheStrings.size() <= CACHE_MAX_SIZE)
        return;

    const auto TO_REMOVE = m_cacheStrings.size() - CACHE_MAX_SIZE;
    m_cacheStrings       = std::vector<std::string>{m_cacheStrings.begin() + TO_REMOVE, m_cacheStrings.end()};
}

void CEntryCache::save() {
    static auto PENABLECACHE = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(g_configManager->m_config.get(), "cache:enabled");

    if (!*PENABLECACHE)
        return;

    trimCache();

    std::ofstream ofs(m_cacheFullPath);

    for (const auto& s : m_cacheStrings) {
        ofs << std::format("{}\n", s);
    }
}