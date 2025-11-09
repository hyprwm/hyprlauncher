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
#include <hyprtoolkit/element/Image.hpp>

#include "../helpers/Memory.hpp"

struct SFinderResult;

class CResultButton {
  public:
    CResultButton();
    ~CResultButton() = default;

    SP<Hyprtoolkit::CRectangleElement> m_background;
    SP<Hyprtoolkit::CImageElement>     m_icon;
    SP<Hyprtoolkit::CNullElement>      m_iconPlaceholder;
    SP<Hyprtoolkit::CRowLayoutElement> m_container;
    SP<Hyprtoolkit::CTextElement>      m_label;

    bool                               m_added = false;

    void                               setActive(bool active);
    void                               setLabel(const std::string& x, const std::string& icon = "");

  private:
    void        updatedFontSize();

    bool        m_active       = false;
    int         m_lastFontSize = 0.F;
    std::string m_lastLabel = "", m_lastIcon = "";
};
