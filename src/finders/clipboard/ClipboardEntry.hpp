#include "../IFinder.hpp"
#include "../../clipboard/ClipboardManager.hpp"
#include "../../helpers/Log.hpp"
#include "../Cache.hpp"

class CClipboardEntry : public IFinderResult {
  public:
    CClipboardEntry(const std::string& content, CClipboardManager* manager, CEntryCache* cache);
    virtual ~CClipboardEntry() = default;

    virtual std::string fuzzable() {
        return m_fuzzable;
    }

    virtual eFinderTypes type() {
        return FINDER_CLIPBOARD;
    }

    virtual uint32_t frequency() {
        return m_frequency;
    }

    virtual void run() {
        Debug::log(LOG, "Running clipboard entry: copying and pasting");
        if (m_pCache) {
            m_pCache->incrementCachedEntry(m_fuzzable);
            m_frequency = m_pCache->getCachedEntry(m_fuzzable);
        }
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
    CEntryCache*       m_pCache;
    uint32_t           m_frequency = 0;
};

inline CClipboardEntry::CClipboardEntry(const std::string& content, CClipboardManager* manager, CEntryCache* cache) :
    m_fuzzable(content), m_pManager(manager), m_pCache(cache), m_frequency(cache ? cache->getCachedEntry(content) : 0) {}
