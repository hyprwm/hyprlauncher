#include "QueryProcessor.hpp"
#include "../ui/UI.hpp"
#include "../config/ConfigManager.hpp"

#include "../finders/desktop/DesktopFinder.hpp"
#include "../finders/unicode/UnicodeFinder.hpp"
#include "../finders/math/MathFinder.hpp"
#include "../finders/font/FontFinder.hpp"
#include "../finders/fs/FsFinder.hpp"

static IFinder* finderForName(const std::string& x) {
    if (x == "desktop")
        return g_desktopFinder.get();
    if (x == "unicode")
        return g_unicodeFinder.get();
    if (x == "math")
        return g_mathFinder.get();
    if (x == "font")
        return g_fontFinder.get();
    if (x == "fs")
        return g_fsFinder.get();
    return nullptr;
}

static std::pair<IFinder*, bool> finderForPrefix(const char x) {
    static auto PDEFAULTFINDER = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:default_finder");

    static auto PDESKTOPPREFIX = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:desktop_prefix");
    static auto PUNICODEPREFIX = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:unicode_prefix");
    static auto PMATHPREFIX    = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:math_prefix");
    static auto PFONTPREFIX    = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:font_prefix");
    static auto PFSPREFIX      = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:fs_prefix");

    if (x == (*PDESKTOPPREFIX)[0])
        return {g_desktopFinder.get(), true};
    if (x == (*PUNICODEPREFIX)[0])
        return {g_unicodeFinder.get(), true};
    if (x == (*PMATHPREFIX)[0])
        return {g_mathFinder.get(), true};
    if (x == (*PFONTPREFIX)[0])
        return {g_fontFinder.get(), true};
    if (x == (*PFSPREFIX)[0])
        return {g_fsFinder.get(), true};
    return {finderForName(*PDEFAULTFINDER), false};
}

CQueryProcessor::CQueryProcessor() {
    m_queryThread = std::thread([this] {
        while (true) {
            SQueryRequest request;

            {
                std::unique_lock lock(m_mutex);
                m_threadCV.wait(lock, [this] { return m_quit || m_pendingQuery.has_value(); });

                if (m_quit)
                    return;

                request = std::move(*m_pendingQuery);
                m_pendingQuery.reset();
            }

            process(std::move(request));
        }
    });
}

CQueryProcessor::~CQueryProcessor() {
    {
        std::lock_guard lock(m_mutex);
        m_quit = true;
        m_pendingQuery.reset();
    }

    m_threadCV.notify_all();
    m_queryThread.join();
}

void CQueryProcessor::scheduleQueryUpdate(const std::string& str) {
    {
        std::lock_guard lock(m_mutex);
        m_pendingQuery = SQueryRequest{
            .query      = str,
            .finder     = m_overrideFinder ? m_overrideFinder : m_selectFinder,
            .generation = ++m_generation,
        };
    }

    m_threadCV.notify_one();
}

void CQueryProcessor::overrideQueryProvider(IFinder* finder) {
    std::lock_guard lock(m_mutex);
    m_overrideFinder = finder;
    m_pendingQuery.reset();
    ++m_generation;
}

void CQueryProcessor::selectQueryProvider(const std::string& finder) {
    std::lock_guard lock(m_mutex);
    m_selectFinder = finderForName(finder);
    m_pendingQuery.reset();
    ++m_generation;
}

// Only run on the query thread.
void CQueryProcessor::process(SQueryRequest&& request) {
    std::lock_guard processingLock(m_processingMutex);

    if (!isCurrentGeneration(request.generation))
        return;

    IFinder*    FINDER = request.finder;
    std::string query  = std::move(request.query);
    bool        eat    = false;

    if (!FINDER) {
        if (query.empty()) {
            // Only show apps on empty query if configured to do so
            static auto PSHOWONOPEN = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(g_configManager->m_config.get(), "general:show_apps_on_open");
            if (*PSHOWONOPEN) {
                static auto PDEFAULTFINDER = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(g_configManager->m_config.get(), "finders:default_finder");
                FINDER                     = finderForName(*PDEFAULTFINDER);
            } else {
                publishResults(request.generation, {});
                return;
            }
        } else {
            const auto [F, e] = finderForPrefix(query[0]);

            FINDER = F;
            eat    = e;

            if (eat && query.size() == 1) {
                query.clear();
                eat = false;
            }
        }
    }

    auto RESULTS = FINDER ? FINDER->getResultsForQuery(eat ? query.substr(1) : query) : std::vector<SFinderResult>{};
    publishResults(request.generation, std::move(RESULTS));
}

void CQueryProcessor::publishResults(uint64_t generation, std::vector<SFinderResult>&& results) {
    if (!g_ui || !g_ui->m_backend)
        return;

    g_ui->m_backend->addIdle([this, generation, results = std::move(results)] mutable {
        bool activate = false;

        {
            std::lock_guard processingLock(m_processingMutex);

            if (!isCurrentGeneration(generation) || !g_ui) {
                results.clear();
                return;
            }

            activate = g_ui->updateResults(std::move(results));
        }

        if (activate && g_ui)
            g_ui->onSelected();
    });
}

bool CQueryProcessor::isCurrentGeneration(uint64_t generation) {
    std::lock_guard lock(m_mutex);
    return generation == m_generation;
}
