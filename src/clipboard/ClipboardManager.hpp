#pragma once

#include <string>
#include <vector>
#include "../config/ConfigManager.hpp"

struct SClipboardHistoryItem {
  std::string originalLine;
  std::string displayLine;
};

class CClipboardManager {
  public:
    CClipboardManager(const SClipboardConfig& config);

    SClipboardConfig                    getConfig() const;
    std::vector<SClipboardHistoryItem>  getHistory();

    void                      copyItem(const std::string& item);
    void                      deleteItem(const std::string& item);

  private:
    SClipboardConfig m_Config;
};
