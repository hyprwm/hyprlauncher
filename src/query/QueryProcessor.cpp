#include "QueryProcessor.hpp"
#include "../ui/UI.hpp"
#include "../config/ConfigManager.hpp"

#include "../finders/desktop/DesktopFinder.hpp"
#include "../finders/unicode/UnicodeFinder.hpp"
#include "../finders/math/MathFinder.hpp"
#include "../finders/font/FontFinder.hpp"
#include "../finders/fs/FsFinder.hpp"

#include <hyprutils/utils/ScopeGuard.hpp>

using namespace Hyprutils::Utils;

static WP<IFinder> finderForName(const std::string& x) {
    if (x == "desktop")
        return g_desktopFinder;
    if (x == "unicode")
        return g_unicodeFinder;
    if (x == "math")
        return g_mathFinder;
    if (x == "fs")
        return g_fsFinder;
    return WP<IFinder>{};
}

static std::pair<WP<IFinder>, bool> finderForPrefix(const char x) {
    static auto PDEFAULTFINDER = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:default_finder");

    static auto PDESKTOPPREFIX = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:desktop_prefix");
    static auto PUNICODEPREFIX = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:unicode_prefix");
    static auto PMATHPREFIX    = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:math_prefix");
    static auto PFONTPREFIX    = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:font_prefix");
    static auto PFSPREFIX      = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:fs_prefix");

    if (x == (*PDESKTOPPREFIX)[0])
        return {g_desktopFinder, true};
    if (x == (*PUNICODEPREFIX)[0])
        return {g_unicodeFinder, true};
    if (x == (*PMATHPREFIX)[0])
        return {g_mathFinder, true};
    if (x == (*PFONTPREFIX)[0])
        return {g_fontFinder, true};
    if (x == (*PFSPREFIX)[0])
        return {g_fsFinder, true};
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

            while (!m_quit && m_newQuery) {
                process();
            }
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
    m_newQuery     = true;
    m_event        = true;
    m_queryStrMutex.unlock();
    m_threadCV.notify_all();
}

void CQueryProcessor::overrideQueryProvider(WP<IFinder> finder) {
    std::lock_guard<std::mutex> lg(m_processingMutex);
    m_overrideFinder = finder;
}

// Only ran on process thread
void CQueryProcessor::process() {
    CScopeGuard x([this] { m_newQuery = false; });

    if (m_quit)
        return;

    m_queryStrMutex.lock();

    std::string query = m_pendingQuery;
    m_pendingQuery    = "";

    m_queryStrMutex.unlock();

    WP<IFinder> FINDER;
    bool        eat = false;

    if (!m_overrideFinder) {
        const auto [F, e] = finderForPrefix(query[0]);

        if (e && query.size() == 1)
            return;

        FINDER = F;
        eat    = e;
    } else
        FINDER = m_overrideFinder;

    if (query.empty() && !m_overrideFinder) {
        if (g_ui)
            g_ui->m_backend->addIdle([] mutable { g_ui->updateResults({}); });
        return;
    }

    auto RESULTS = FINDER ? FINDER->getResultsForQuery(eat ? query.substr(1) : query) : std::vector<SFinderResult>{};

    if (g_ui && g_ui->m_backend)
        g_ui->m_backend->addIdle([r = std::move(RESULTS)] mutable { g_ui->updateResults(std::move(r)); });
}
