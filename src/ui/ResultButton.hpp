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

#include "../helpers/Memory.hpp"

struct SFinderResult;

class CResultButton {
  public:
    CResultButton();
    ~CResultButton() = default;

    SP<Hyprtoolkit::CRectangleElement> m_background;
    SP<Hyprtoolkit::CNullElement>      m_container;
    SP<Hyprtoolkit::CTextElement>      m_label;

    bool                               m_added = false;

    void                               setActive(bool active);
    void                               setLabel(const std::string& x);

  private:
    void        updatedFontSize();

    bool        m_active       = false;
    int         m_lastFontSize = 0.F;
    std::string m_lastLabel    = "";
};
