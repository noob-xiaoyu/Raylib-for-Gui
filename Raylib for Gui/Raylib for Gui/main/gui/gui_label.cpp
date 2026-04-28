#include "gui_label.h"
#include "../backends/font/FontManager.h"

// ============================================================================
// GUILabel 实现
// ============================================================================
GUILabel::GUILabel(int id, int zIndex, const std::string& text, float x, float y,
                   int fontSize, RLColor color)
    : GUIWidget(id, zIndex)
    , m_text(text)
    , m_fontSize(fontSize)
    , m_spacing(2)
{
    setPosition(x, y);
    setColor(color);
    // 自动根据文本调整大小
    RLVector2 sz = MeasureTextCustom(m_text.c_str(), m_fontSize, m_spacing);
    setSize(sz.x, sz.y);
}

void GUILabel::draw() {
    if (!m_visible) return;
    auto pos = getAbsolutePosition();
    DrawTextCustom(m_text.c_str(), { pos.x, pos.y }, m_fontSize, m_color, m_spacing);
}

void GUILabel::setText(const std::string& text) {
    m_text = text;
    RLVector2 sz = MeasureTextCustom(m_text.c_str(), m_fontSize, m_spacing);
    setSize(sz.x, sz.y);
}

void GUILabel::setFontSize(int fontSize) {
    m_fontSize = fontSize;
    RLVector2 sz = MeasureTextCustom(m_text.c_str(), m_fontSize, m_spacing);
    setSize(sz.x, sz.y);
}

void GUILabel::setSpacing(int spacing) {
    m_spacing = spacing;
}
