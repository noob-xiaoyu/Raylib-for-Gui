#include "gui_toggle.h"
#include "raylib.h"
#include "../backends/font/FontManager.h"
#include <algorithm>

// ============================================================================
// GUIToggle 实现
// ============================================================================
GUIToggle::GUIToggle(int id, int zIndex, const std::string& label, float x, float y,
                     bool initialState)
    : GUIWidget(id, zIndex)
    , m_checked(initialState)
    , m_prevChecked(initialState)
    , m_animT(initialState ? 1.0f : 0.0f)
    , m_label(label)
    , m_trackOnColor(RLColor{ 100, 200, 100, 255 })
    , m_trackOffColor(RLColor{ 80, 80, 110, 255 })
    , m_thumbOnColor(RLColor{ 240, 240, 255, 255 })
    , m_thumbOffColor(RLColor{ 200, 200, 220, 255 })
    , m_thumbHoverColor(RLColor{ 255, 255, 255, 255 })
    , m_labelColor(WHITE)
    , m_labelSize(16)
    , m_trackWidth(44.0f)
    , m_trackHeight(24.0f)
    , m_thumbRadius((m_trackHeight - k_thumbPadding * 2.0f) / 2.0f)
    , m_roundness(m_trackHeight / 2.0f)
{
    // 总宽度 = track + 间距 + 标签文字宽度（近似，实际 draw 时精确测量）
    float totalWidth = m_trackWidth + k_labelGap + 100.0f; // 粗略，draw 时会调整
    setPosition(x, y);
    setSize(totalWidth, m_trackHeight);
}

void GUIToggle::draw() {
    if (!m_visible) return;
    auto pos = getAbsolutePosition();

    // === 滑轨 (track) ===
    float trackX = pos.x;
    float trackY = pos.y + (m_height - m_trackHeight) / 2.0f;
    RLRectangle trackRect{ trackX, trackY, m_trackWidth, m_trackHeight };

    // 颜色插值（根据动画进度）
    RLColor trackColor = RLColorLerp(m_trackOffColor, m_trackOnColor, m_animT);
    RLDrawRectangleRounded(trackRect, m_roundness / (m_trackHeight / 2.0f), 8, trackColor);

    // 边框
    RLDrawRectangleRoundedLinesEx(trackRect, m_roundness / (m_trackHeight / 2.0f), 8, 1.5f,
                                  RLColor{ 100, 100, 150, 180 });

    // === 圆形滑块 (thumb) ===
    float thumbTravel = m_trackWidth - m_thumbRadius * 2.0f - k_thumbPadding * 2.0f;
    float thumbCX = trackX + m_thumbRadius + k_thumbPadding + thumbTravel * m_animT;
    float thumbCY = trackY + m_trackHeight / 2.0f;

    // thumb 颜色：hover 时高亮，否则根据开关状态
    RLColor thumbColor = m_hovered ? m_thumbHoverColor :
        (m_checked ? m_thumbOnColor : m_thumbOffColor);

    RLDrawCircle(static_cast<int>(thumbCX), static_cast<int>(thumbCY),
                 m_thumbRadius, thumbColor);

    // thumb 阴影/描边
    RLDrawCircleLines(static_cast<int>(thumbCX), static_cast<int>(thumbCY),
                      m_thumbRadius, RLColor{ 60, 60, 90, 120 });

    // === 标签 ===
    if (!m_label.empty()) {
        RLVector2 textSize = MeasureTextCustom(m_label.c_str(), m_labelSize, 2);
        float tx = trackX + m_trackWidth + k_labelGap;
        float ty = trackY + (m_trackHeight - textSize.y) / 2.0f;
        DrawTextCustom(m_label.c_str(), { tx, ty }, m_labelSize, m_labelColor, 2);

        // 更新控件宽度以包含完整标签
        // 注意：这里不修改 m_width 以避免布局漂移，子类可自行保证宽度足够
    }
}

void GUIToggle::update(float dt) {
    // 动画插值
    float target = m_checked ? 1.0f : 0.0f;
    if (std::abs(m_animT - target) > 0.001f) {
        m_animT += (target - m_animT) * std::min(k_animSpeed * dt, 1.0f);
        if (std::abs(m_animT - target) < 0.001f) {
            m_animT = target;
        }
    } else {
        m_animT = target;
    }
}

void GUIToggle::onFocus() {
    // 聚焦时可添加高亮边框，由外部渲染
}

void GUIToggle::onBlur() {
    // 失焦无特殊行为
}

bool GUIToggle::containsPoint(float px, float py) const {
    auto pos = getAbsolutePosition();
    float trackY = pos.y + (m_height - m_trackHeight) / 2.0f;

    // 轨道区域
    if (px >= pos.x && px <= pos.x + m_trackWidth &&
        py >= trackY && py <= trackY + m_trackHeight)
        return true;

    // 标签文本区域
    if (!m_label.empty()) {
        RLVector2 textSize = MeasureTextCustom(m_label.c_str(), m_labelSize, 2);
        float labelX = pos.x + m_trackWidth + k_labelGap;
        float labelY = trackY + (m_trackHeight - textSize.y) / 2.0f;
        if (px >= labelX && px <= labelX + textSize.x &&
            py >= labelY && py <= labelY + textSize.y)
            return true;
    }

    return false;
}

void GUIToggle::onMouseEnter() {
    if (m_visible && m_enabled)
        m_hovered = true;
}

void GUIToggle::onMouseLeave() {
    m_hovered = false;
}

void GUIToggle::onClick(const MouseState& state) {
    if (m_visible && m_enabled) {
        toggle();
    }
}

RLRectangle GUIToggle::getFocusRect() const {
    // 焦点框覆盖轨道 + 标签文本区域
    auto pos = getAbsolutePosition();
    float trackY = pos.y + (m_height - m_trackHeight) / 2.0f;

    if (!m_label.empty()) {
        RLVector2 textSize = MeasureTextCustom(m_label.c_str(), m_labelSize, 2);
        float labelX = pos.x + m_trackWidth + k_labelGap;
        float totalW = (labelX + textSize.x) - pos.x;
        float maxH = std::max(m_trackHeight, textSize.y);
        float centerY = trackY + (m_trackHeight - maxH) / 2.0f;
        return { pos.x, centerY, totalW, maxH };
    }

    return { pos.x, trackY, m_trackWidth, m_trackHeight };
}

// ============================================================================
// 状态操作
// ============================================================================
void GUIToggle::setChecked(bool checked) {
    if (m_checked != checked) {
        m_checked = checked;
        if (m_onToggle) {
            m_onToggle(m_checked);
        }
    }
}

void GUIToggle::toggle() {
    m_checked = !m_checked;
    if (m_onToggle) {
        m_onToggle(m_checked);
    }
}

// ============================================================================
// 外观设置
// ============================================================================
void GUIToggle::setLabel(const std::string& label) {
    m_label = label;
}

void GUIToggle::setOnToggle(ToggleCallback callback) {
    m_onToggle = callback;
}

void GUIToggle::setTrackOnColor(RLColor color) {
    m_trackOnColor = color;
}

void GUIToggle::setTrackOffColor(RLColor color) {
    m_trackOffColor = color;
}

void GUIToggle::setThumbOnColor(RLColor color) {
    m_thumbOnColor = color;
}

void GUIToggle::setThumbOffColor(RLColor color) {
    m_thumbOffColor = color;
}

void GUIToggle::setThumbHoverColor(RLColor color) {
    m_thumbHoverColor = color;
}

void GUIToggle::setLabelColor(RLColor color) {
    m_labelColor = color;
}

void GUIToggle::setLabelSize(int size) {
    m_labelSize = size;
}

void GUIToggle::setTrackWidth(float width) {
    m_trackWidth = width;
    m_thumbRadius = (m_trackHeight - k_thumbPadding * 2.0f) / 2.0f;
    m_roundness = m_trackHeight / 2.0f;
}

void GUIToggle::setTrackHeight(float height) {
    m_trackHeight = height;
    m_thumbRadius = (m_trackHeight - k_thumbPadding * 2.0f) / 2.0f;
    m_roundness = m_trackHeight / 2.0f;
}
