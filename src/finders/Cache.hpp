#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <filesystem>

class CEntryCache {
  public:
    CEntryCache(const std::string& name);
    ~CEntryCache() = default;

    bool     good();
    uint32_t getCachedEntry(const std::string& entry);
    void     incrementCachedEntry(const std::string& entry);

  private:
    void trimCache();
    void save();

    bool m_good = false;

    struct SCacheEntry {
        uint32_t value = 0;
    };

    std::unordered_map<std::string, SCacheEntry> m_cache;
    std::vector<std::string>                     m_cacheStrings;
    std::filesystem::path                        m_cacheFullPath;
};
