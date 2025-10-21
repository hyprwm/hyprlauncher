#pragma once

#include <string>
#include <vector>
#include "../config/ConfigManager.hpp"

struct SClipboardHistoryItem {
  std::string original_line;
  std::string display_line;
};

class CClipboardManager {
  public:
    CClipboardManager(const SClipboardConfig& config);

    SClipboardConfig                    getConfig() const;
    std::vector<SClipboardHistoryItem>  getHistory();

    void                      copyItem(const std::string& item);
    void                      deleteItem(const std::string& item);

  private:
    SClipboardConfig m_sConfig;
};
