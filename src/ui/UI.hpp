#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Textbox.hpp>

#include <hyprtoolkit/system/Icons.hpp>

#include "../helpers/Memory.hpp"
#include "../finders/IFinder.hpp"

class CResultButton;

class CUI {
  public:
    CUI(bool open);
    ~CUI();

    void run();
    void setWindowOpen(bool open);
    bool windowOpen();

    // WARNING: has to be called from within the main thread. NOT thread safe!!
    void updateResults(std::vector<SFinderResult>&& results);

    void updateActive();

  private:
    void                                  onSelected();

    SP<Hyprtoolkit::CBackend>             m_backend;
    SP<Hyprtoolkit::IWindow>              m_window;
    SP<Hyprtoolkit::CRectangleElement>    m_background;
    SP<Hyprtoolkit::CColumnLayoutElement> m_layout;

    SP<Hyprtoolkit::CTextboxElement>      m_inputBox;
    SP<Hyprtoolkit::CRectangleElement>    m_hr;
    SP<Hyprtoolkit::CScrollAreaElement>   m_scrollArea;
    SP<Hyprtoolkit::CColumnLayoutElement> m_resultsLayout;

    std::vector<SFinderResult>            m_currentResults;
    std::vector<SP<CResultButton>>        m_resultButtons;

    bool                                  m_open            = false;
    bool                                  m_openByDefault   = true;
    size_t                                m_activeElementId = 0;

    friend class CQueryProcessor;
    friend class CResultButton;
};

inline UP<CUI> g_ui;
