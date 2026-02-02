#include "UI.hpp"
#include "ResultButton.hpp"

#include "../finders/desktop/DesktopFinder.hpp"
#include "../query/QueryProcessor.hpp"
#include "../socket/ServerSocket.hpp"
#include "../helpers/Log.hpp"
#include "../config/ConfigManager.hpp"
#include "../i18n/Engine.hpp"

#include <hyprutils/string/String.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>

using namespace Hyprutils::Math;
using namespace Hyprutils::String;

constexpr const size_t MAX_RESULTS_IN_LAUNCHER = 50;

CUI::CUI(bool open) : m_openByDefault(open) {
    static auto PGRABFOCUS  = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(g_configManager->m_config.get(), "general:grab_focus");
    static auto PWINDOWSIZE = Hyprlang::CSimpleConfigValue<Hyprlang::VEC2>(g_configManager->m_config.get(), "ui:window_size");

    m_backend = Hyprtoolkit::IBackend::create();

    m_background = Hyprtoolkit::CRectangleBuilder::begin()
                       ->color([this] { return m_backend->getPalette()->m_colors.background; })
                       ->rounding(10)
                       ->borderColor([this] { return m_backend->getPalette()->m_colors.accent.darken(0.2F); })
                       ->borderThickness(1)
                       ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                       ->commence();

    m_layout =
        Hyprtoolkit::CColumnLayoutBuilder::begin()->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->gap(4)->commence();
    m_layout->setMargin(4);

    m_inputBox = Hyprtoolkit::CTextboxBuilder::begin()
                     ->placeholder(I18n::localize(I18n::TXT_KEY_SEARCH_SOMETHING, {}))
                     ->onTextEdited([](SP<Hyprtoolkit::CTextboxElement>, const std::string& query) { g_queryProcessor->scheduleQueryUpdate(query); })
                     ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, 28.F}})
                     ->multiline(false)
                     ->commence();

    m_hr = Hyprtoolkit::CRectangleBuilder::begin()
               ->color([this] { return m_backend->getPalette()->m_colors.accent.darken(0.2F); })
               ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {0.8F, 1.F}})
               ->commence();
    m_hr->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    m_hr->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);

    m_scrollArea = Hyprtoolkit::CScrollAreaBuilder::begin()
                       ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, 10.F}})
                       ->scrollY(true)
                       ->commence();
    m_scrollArea->setGrow(true);

    m_resultsLayout =
        Hyprtoolkit::CColumnLayoutBuilder::begin()->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO, {1, 1}})->gap(2)->commence();

    m_background->addChild(m_layout);

    m_layout->addChild(m_inputBox);
    m_layout->addChild(m_hr);
    m_layout->addChild(m_scrollArea);

    m_scrollArea->addChild(m_resultsLayout);

    //
    m_window = Hyprtoolkit::CWindowBuilder::begin()
                   ->appClass("hyprlauncher")
                   ->type(Hyprtoolkit::HT_WINDOW_LAYER)
                   ->preferredSize({(*PWINDOWSIZE).x, (*PWINDOWSIZE).y})
                   ->anchor(1 | 2 | 4 | 8)
                   ->exclusiveZone(-1)
                   ->layer(3)
                   ->kbInteractive(*PGRABFOCUS ? 1 : 2)
                   ->commence();

    m_window->m_rootElement->addChild(m_background);

    m_window->m_events.keyboardKey.listenStatic([this](Hyprtoolkit::Input::SKeyboardKeyEvent e) {
        if (e.xkbKeysym == XKB_KEY_Escape)
            setWindowOpen(false);
        else if (e.xkbKeysym == XKB_KEY_Down) {
            if (m_activeElementId + 1 < m_currentResults.size())
                m_activeElementId++;
            updateActive();
        } else if (e.xkbKeysym == XKB_KEY_Up) {
            if (m_activeElementId > 0)
                m_activeElementId--;
            updateActive();
        } else if (e.xkbKeysym == XKB_KEY_Return || e.xkbKeysym == XKB_KEY_KP_Enter)
            onSelected();
    });
}

CUI::~CUI() = default;

void CUI::run() {
    m_resultButtons.reserve(MAX_RESULTS_IN_LAUNCHER);
    for (size_t i = 0; i < MAX_RESULTS_IN_LAUNCHER; ++i) {
        auto b     = m_resultButtons.emplace_back(makeShared<CResultButton>());
        b->m_added = true;
        m_resultsLayout->addChild(b->m_background);
    }

    if (m_openByDefault)
        setWindowOpen(true);

    if (g_serverIPCSocket->m_socket) {
        m_backend->addFd(g_serverIPCSocket->m_socket->extractLoopFD(), [] {
            Debug::log(TRACE, "got an ipc event");
            g_serverIPCSocket->m_socket->dispatchEvents(false);
        });
    }

    m_backend->addFd(g_configManager->m_inotifyFd.get(), [] { g_configManager->onInotifyEvent(); });
    m_backend->addFd(g_desktopFinder->m_inotifyFd.get(), [] { g_desktopFinder->onInotifyEvent(); });

    m_backend->enterLoop();
}

void CUI::setWindowOpen(bool open) {
    if (open == m_open)
        return;

    m_open = open;

    if (open) {
        m_inputBox->rebuild()->defaultText("")->commence();

        for (const auto& b : m_resultButtons) {
            b->setLabel("", "", std::nullopt, false);
            b->setActive(false);
        }

        m_window->open();

        m_inputBox->focus();

        g_queryProcessor->scheduleQueryUpdate("");
    } else {
        m_window->close();
        g_queryProcessor->overrideQueryProvider(WP<IFinder>{});
    }

    g_serverIPCSocket->sendOpenState(open);
}

void CUI::onSelected() {
    if (m_currentResults.size() <= m_activeElementId)
        return;
    g_serverIPCSocket->sendSelectionMade(m_currentResults.at(m_activeElementId).result->name());
    m_currentResults.at(m_activeElementId).result->run();
    setWindowOpen(false);
    g_queryProcessor->overrideQueryProvider(WP<IFinder>{});
}

bool CUI::windowOpen() {
    return m_open;
}

void CUI::updateResults(std::vector<SFinderResult>&& results) {
    m_currentResults = std::move(results);

    m_activeElementId = 0;

    for (size_t i = 0; i < m_resultButtons.size(); ++i) {
        if (m_currentResults.size() <= i)
            m_resultButtons[i]->setLabel("", "", std::nullopt, false);
        else
            m_resultButtons[i]->setLabel(m_currentResults[i].label, m_currentResults[i].icon, m_currentResults[i].overrideFont, m_currentResults[i].hasIcon);
    }

    updateActive();
}

void CUI::updateActive() {
    for (size_t i = 0; i < m_resultButtons.size(); ++i) {
        auto& b = m_resultButtons[i];
        b->setActive(i == m_activeElementId);
        if (i >= m_currentResults.size() && b->m_added)
            m_resultsLayout->removeChild(b->m_background);
        else if (i < m_currentResults.size() && !b->m_added)
            m_resultsLayout->addChild(b->m_background);
        b->m_added = i < m_currentResults.size();
    }

    // fit the scroll area
    const float CURRENT_SCROLL_Y = m_scrollArea->getCurrentScroll().y;
    const float BUTTON_HEIGHT    = m_resultButtons[0]->m_background->size().y + 2 /* gap */;

    const float MIN_SCROLL_TO_SEE = (BUTTON_HEIGHT * (m_activeElementId + 1)) - (m_scrollArea->size().y);
    const float MAX_SCROLL_TO_SEE = (BUTTON_HEIGHT * m_activeElementId);

    if (MAX_SCROLL_TO_SEE <= MIN_SCROLL_TO_SEE)
        return; // wtf??

    m_scrollArea->setScroll({0.F, std::clamp(CURRENT_SCROLL_Y, MIN_SCROLL_TO_SEE, MAX_SCROLL_TO_SEE)});
}
