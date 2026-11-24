#include "ResultButton.hpp"
#include "../finders/IFinder.hpp"

#include "UI.hpp"
#include "../helpers/Icons.hpp"
#include <hyprtoolkit/element/Line.hpp>
#include <cmath>

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
                       ->rounding(g_ui->m_backend->getPalette()->m_vars.smallRounding)
                       ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, BG_HEIGHT}})
                       ->commence();

    m_container =
        Hyprtoolkit::CRowLayoutBuilder::begin()->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->gap(4)->commence();
    m_container->setMargin(4);

    m_icon            = Hyprtoolkit::CImageBuilder::begin()
                            ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {0.7F * BG_HEIGHT, 0.7F * BG_HEIGHT}})
                            ->commence();
    m_iconPlaceholder = Hyprtoolkit::CNullBuilder::begin()
                            ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {0.7F * BG_HEIGHT, 0.7F * BG_HEIGHT}})
                            ->commence();
    m_icon->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    m_icon->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);
    m_iconPlaceholder->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    m_iconPlaceholder->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);

    // A terminal outline remains crisp at any size and follows the text color.
    auto fallbackFrame = Hyprtoolkit::CRectangleBuilder::begin()
                             ->color([] { return Hyprtoolkit::CHyprColor{0.F, 0.F, 0.F, 0.F}; })
                             ->borderColor([] { return g_ui->m_backend->getPalette()->m_colors.text; })
                             ->borderThickness(2)
                             ->rounding(4)
                             ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, {0.9F, 0.75F}})
                             ->commence();
    fallbackFrame->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    fallbackFrame->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_CENTER, true);
    m_iconPlaceholder->addChild(fallbackFrame);
    for (auto points : {std::vector<Hyprutils::Math::Vector2D>{{0.2, 0.28}, {0.4, 0.5}, {0.2, 0.72}}, std::vector<Hyprutils::Math::Vector2D>{{0.52, 0.72}, {0.8, 0.72}}}) {
        auto line = Hyprtoolkit::CLineBuilder::begin()
                        ->color([] { return g_ui->m_backend->getPalette()->m_colors.text; })
                        ->thick(2)
                        ->points(std::move(points))
                        ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                        ->commence();
        line->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
        fallbackFrame->addChild(line);
    }
    m_background->setRepositioned([this] {
        if (Hyprtoolkit::CFontSize{Hyprtoolkit::CFontSize::HT_FONT_TEXT}.ptSize() != m_lastFontSize)
            updatedFontSize();
        updateIcon();
    });

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

void CResultButton::setLabel(const std::string& x, const std::string& icon, std::optional<std::string> font, bool canHaveIcon, const std::vector<std::string>& fallbackIcons) {

    if (const auto FONT_SIZE = Hyprtoolkit::CFontSize{Hyprtoolkit::CFontSize::HT_FONT_TEXT}.ptSize(); FONT_SIZE != m_lastFontSize)
        updatedFontSize();

    if (!m_iconInitialized || icon != m_lastIcon || canHaveIcon != m_canHaveIcon || fallbackIcons != m_fallbackIcons) {
        m_lastIcon        = icon;
        m_fallbackIcons   = fallbackIcons;
        m_canHaveIcon     = canHaveIcon;
        m_iconInitialized = false;
    }
    updateIcon();

    if (x != m_lastLabel) {
        m_lastLabel = x;

        m_label->rebuild()->text(std::string{x})->fontFamily(font.value_or(g_ui->m_backend->getPalette()->m_vars.fontFamily))->commence();
    }
}

void CResultButton::updatedFontSize() {
    if (!m_background)
        return;

    const auto FONT_SIZE = Hyprtoolkit::CFontSize{Hyprtoolkit::CFontSize::HT_FONT_TEXT}.ptSize();

    m_background->rebuild()->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, (FONT_SIZE * 2.F) + 4.F}})->commence();

    const auto ICON_SIZE = 0.7F * ((FONT_SIZE * 2.F) + 4.F);
    m_icon->rebuild()->size({Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {ICON_SIZE, ICON_SIZE}})->commence();
    m_iconPlaceholder->rebuild()->size({Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, {ICON_SIZE, ICON_SIZE}})->commence();
    m_lastFontSize = FONT_SIZE;
}

void CResultButton::updateIcon() {
    const auto  SCALE  = g_ui->m_window ? g_ui->m_window->scale() : 1.F;
    const int   PIXELS = std::ceil(0.7F * ((m_lastFontSize * 2.F) + 4.F) * SCALE);
    const auto& THEME  = g_ui->m_backend->getPalette()->m_vars.iconTheme;
    if (m_iconInitialized && PIXELS == m_lastIconPixels && THEME == m_lastIconTheme)
        return;

    m_iconInitialized = true;
    m_lastIconPixels  = PIXELS;
    m_lastIconTheme   = THEME;
    m_container->clearChildren();
    if (m_canHaveIcon) {
        const auto ICON = applicationIcon(*g_ui->m_backend->systemIcons(), m_lastIcon, m_fallbackIcons, m_canHaveIcon);
        if (!ICON)
            m_container->addChild(m_iconPlaceholder);
        else {
            m_icon->rebuild()->icon(ICON)->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_CONTAIN)->commence();
            m_container->addChild(m_icon);
        }
    }
    m_container->addChild(m_label);
}
