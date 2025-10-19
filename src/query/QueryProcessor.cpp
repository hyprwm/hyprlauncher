#include "QueryProcessor.hpp"
#include "../ui/UI.hpp"
#include "../config/ConfigManager.hpp"
#include "../finders/desktop/DesktopFinder.hpp"
#include "../finders/unicode/UnicodeFinder.hpp"
#include "../finders/math/MathFinder.hpp"
#include "../finders/clipboard/ClipboardFinder.hpp"

static WP<IFinder> finderForName(const std::string& x) {
    if (x == "desktop")
        return g_desktopFinder;
    if (x == "unicode")
        return g_unicodeFinder;
    if (x == "math")
        return g_mathFinder;
    if (x == "clipboard")
        return g_clipboardFinder;
    return WP<IFinder>{};
}

static std::pair<WP<IFinder>, bool> finderForPrefix(const char x) {
    static auto PDEFAULTFINDER = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:default_finder");
    static auto PDESKTOPPREFIX = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:desktop_prefix");
    static auto PUNICODEPREFIX = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:unicode_prefix");
    static auto PMATHPREFIX    = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:math_prefix");

    if (x == (*PDESKTOPPREFIX)[0])
        return {g_desktopFinder, true};
    if (x == (*PUNICODEPREFIX)[0])
        return {g_unicodeFinder, true};
    if (x == (*PMATHPREFIX)[0])
        return {g_mathFinder, true};
    return {finderForName(*PDEFAULTFINDER), false};
}

CQueryProcessor::CQueryProcessor() {
    m_queryThread = std::thread([this] {
        while (!m_quit) {
            std::unique_lock lk(m_threadMutex);
            m_threadCV.wait(lk, [this] { return m_event; });
            m_event = false;

            if (m_quit)
                break;

            std::lock_guard<std::mutex> lg(m_processingMutex);

            m_queryStrMutex.lock();

            std::string query = m_pendingQuery;
            m_pendingQuery    = "";

            m_queryStrMutex.unlock();

            WP<IFinder> FINDER;
            bool        eat = false;

            if (!m_overrideFinder) {
                const auto [F, e] = finderForPrefix(query[0]);

                if (e && query.size() == 1)
                    continue;

                FINDER = F;
                eat    = e;
            } else
                FINDER = m_overrideFinder;

            if (query.empty() && !m_overrideFinder)
                continue;

            auto RESULTS = FINDER ? FINDER->getResultsForQuery(eat ? query.substr(1) : query) : std::vector<SFinderResult>{};

            if (g_ui && g_ui->m_backend)
                g_ui->m_backend->addIdle([r = std::move(RESULTS)] mutable { g_ui->updateResults(std::move(r)); });
        }
    });
}

CQueryProcessor::~CQueryProcessor() {
    m_quit         = true;
    m_pendingQuery = "exit";
    m_event        = true;
    m_threadCV.notify_all();
    m_queryThread.join();
}

void CQueryProcessor::scheduleQueryUpdate(const std::string& str) {
    m_queryStrMutex.lock();
    m_pendingQuery = str;
    m_event        = true;
    m_queryStrMutex.unlock();
    m_threadCV.notify_all();
}

void CQueryProcessor::overrideQueryProvider(WP<IFinder> finder) {
    std::lock_guard<std::mutex> lg(m_processingMutex);
    m_overrideFinder = finder;
}
