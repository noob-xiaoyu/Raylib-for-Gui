#include "gui_panel.h"
#include "raylib.h"

// ============================================================================
// GUIPanel 实现（支持软阴影）
// ============================================================================
GUIPanel::GUIPanel(int id, int zIndex, float x, float y, float width, float height)
    : GUIWidget(id, zIndex)
    , m_borderColor(RLColor{ 80, 80, 120, 255 })
    , m_borderWidth(1.0f)
    , m_roundness(0.0f)
    , m_shadowStrength(0.0f)
    , m_shadowColor(RLColor{ 0, 0, 0, 255 })
    , m_shadowRadius(16.0f)
{
    setPosition(x, y);
    setSize(width, height);
    setColor(RLColor{ 30, 30, 50, 200 });
}

void GUIPanel::draw() {
    if (!m_visible) return;
    auto pos = getAbsolutePosition();

    RLRectangle rec{ pos.x, pos.y, m_width, m_height };

    // 先绘制软阴影（在面板之下）
    if (m_shadowStrength > 0.01f) {
        float radiusPx = m_shadowRadius;
        // 如果 roundness 很小，用阴影自己的半径；否则用面板圆角
        if (m_roundness > 0.01f) {
            float roundPx = m_roundness * ((m_width < m_height) ? m_width : m_height) / 2.0f;
            radiusPx = (roundPx > radiusPx) ? roundPx : radiusPx;
        }
        DrawSoftShadowRounded(rec, radiusPx, m_shadowStrength, m_shadowColor);
    }

    // 面板填充
    RLDrawRectangleRounded(rec, m_roundness, 10, m_color);

    // 边框
    if (m_borderWidth > 0.0f) {
        RLDrawRectangleRoundedLinesEx(rec, m_roundness, 10, m_borderWidth, m_borderColor);
    }
}

void GUIPanel::setBorderColor(RLColor color) {
    m_borderColor = color;
}

void GUIPanel::setBorderWidth(float width) {
    m_borderWidth = width;
}

void GUIPanel::setRoundness(float roundness) {
    m_roundness = roundness;
}

void GUIPanel::setShadowStrength(float strength) {
    m_shadowStrength = Clamp01(strength);
}

void GUIPanel::setShadowColor(RLColor color) {
    m_shadowColor = color;
}

void GUIPanel::setShadowRadius(float radius) {
    m_shadowRadius = radius;
}
