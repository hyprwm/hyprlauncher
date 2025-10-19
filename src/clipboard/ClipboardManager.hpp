#pragma once

#include <string>
#include <vector>

struct SClipboardConfig {
    std::string list_cmd   = "cliphist list";
    std::string copy_cmd   = "cliphist decode | wl-copy";
    std::string delete_cmd = "cliphist delete";
    std::string paste_cmd  = "wtype -M shift -P insert -m shift";
};

class CClipboardManager {
  public:
    CClipboardManager(const SClipboardConfig& config);

    SClipboardConfig          getConfig() const;
    std::vector<std::string>  getHistory();

    void                      executeCommand(const std::string& command, const std::string& arg);
    void                      copyItem(const std::string& item);
    void                      paste();
    void                      deleteItem(const std::string& item);

  private:
    SClipboardConfig m_sConfig;
};
