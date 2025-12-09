#include "ResultButton.hpp"
#include "../finders/IFinder.hpp"

#include "UI.hpp"

CResultButton::CResultButton() {
    const auto FONT_SIZE = Hyprtoolkit::CFontSize{Hyprtoolkit::CFontSize::HT_FONT_TEXT}.ptSize();
    m_lastFontSize       = FONT_SIZE;

    const auto BG_HEIGHT = (FONT_SIZE * 2.F) + 4.F;

    m_background = Hyprtoolkit::CRectangleBuilder::begin()
                       ->color([]() {
                           auto c = g_ui->m_backend->getPalette()->m_colors.accent.darken(0.3F);
                           c.a    = 0.F;
                           return c;
                       })
                       ->rounding(4)
                       ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, BG_HEIGHT}})
                       ->commence();

    m_container =
        Hyprtoolkit::CRowLayoutBuilder::begin()->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->gap(4)->commence();
    m_container->setMargin(4);

    m_icon = Hyprtoolkit::CImageBuilder::begin()
                 ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {0.7F * BG_HEIGHT, 0.7F * BG_HEIGHT}})
                 ->commence();
    m_iconPlaceholder = Hyprtoolkit::CNullBuilder::begin()
                            ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {0.7F * BG_HEIGHT, 0.7F * BG_HEIGHT}})
                            ->commence();
    m_icon->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    m_icon->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);
    m_iconPlaceholder->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    m_iconPlaceholder->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);

    m_label = Hyprtoolkit::CTextBuilder::begin()
                  ->text(std::string{m_lastLabel})
                  ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                  ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                  ->commence();

    m_label->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    m_label->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_LEFT, true);
    m_label->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);

    m_background->addChild(m_container);
    m_container->addChild(m_label);
}

void CResultButton::setActive(bool active) {
    if (active == m_active)
        return;

    m_active = active;

    m_background->rebuild()
        ->color([this]() {
            auto c = g_ui->m_backend->getPalette()->m_colors.accent.darken(0.3F);
            c.a    = m_active ? 0.4F : 0.F;
            return c;
        })
        ->commence();
}

void CResultButton::setLabel(const std::string& x, const std::string& icon, const std::string& font) {

    if (const auto FONT_SIZE = Hyprtoolkit::CFontSize{Hyprtoolkit::CFontSize::HT_FONT_TEXT}.ptSize(); FONT_SIZE != m_lastFontSize)
        updatedFontSize();

    if (icon != m_lastIcon) {
        m_lastIcon = icon;

        auto iconDescription = g_ui->m_backend->systemIcons()->lookupIcon(icon);

        m_container->clearChildren();

        if (!iconDescription || !iconDescription->exists()) {
            m_container->addChild(m_iconPlaceholder);
            m_container->addChild(m_label);
        } else {
            m_icon->rebuild()->icon(iconDescription)->commence();
            m_container->addChild(m_icon);
            m_container->addChild(m_label);
        }
    }

    if (x != m_lastLabel) {
        m_lastLabel = x;

        m_label->rebuild()->text(std::string{x})->fontFamily(std::string{font})->commence();
    }
}

void CResultButton::updatedFontSize() {
    if (!m_background)
        return;

    const auto FONT_SIZE = Hyprtoolkit::CFontSize{Hyprtoolkit::CFontSize::HT_FONT_TEXT}.ptSize();

    m_background->rebuild()->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, (FONT_SIZE * 2.F) + 4.F}})->commence();

    m_lastFontSize = FONT_SIZE;
}
