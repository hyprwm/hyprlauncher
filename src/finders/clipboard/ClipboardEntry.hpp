#include "../IFinder.hpp"
#include "../../clipboard/ClipboardManager.hpp"
#include "../../helpers/Log.hpp"

class CClipboardEntry : public IFinderResult {
  public:
    CClipboardEntry(const std::string& content, CClipboardManager* manager) : m_fuzzable(content), m_pManager(manager) {}
    virtual ~CClipboardEntry() = default;

    virtual std::string fuzzable() {
        return m_fuzzable;
    }

    virtual eFinderTypes type() {
        return FINDER_CLIPBOARD;
    }

    virtual void run() {
        Debug::log(LOG, "Running clipboard entry: copying and pasting");
        m_pManager->copyItem(m_fuzzable);
        m_pManager->paste();
    }

    virtual void remove() {
        Debug::log(LOG, "Removing clipboard entry from history");
        m_pManager->deleteItem(m_fuzzable);
    }

  private:
    std::string        m_fuzzable;
    CClipboardManager* m_pManager;
};
