#include "gui_slider.h"
#include "../backends/font/FontManager.h"
#include "../backends/gui/gui_manager.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

// ============================================================================
// 构造/析构
// ============================================================================
GUISlider::GUISlider(int id, int zIndex, float x, float y, float width, float height,
                     SliderType type)
    : GUIWidget(id, zIndex)
    , m_type(type)
{
    setPosition(x, y);
    setSize(width, height);
}

GUISlider::~GUISlider() {
    // 所有子控件（m_inputBox / m_popupInput）的所有权归 GUIManager 管理，
    // GUIManager 析构时会自动销毁它们。此处仅清空原始指针，避免 double-free。
    // 注意：不要在析构函数中调用 GUIManager::removeWidget()，
    // 因为当 GUIManager 正在销毁 m_widgets map 时，m_inputBox 可能已被销毁。
    m_inputBox = nullptr;
    m_popupInput = nullptr;
}

// ============================================================================
// 值操作
// ============================================================================
void GUISlider::SetValue(double val) {
    val = std::clamp(val, m_min, m_max);
    if (m_useStep && m_step > 0) {
        val = std::round((val - m_min) / m_step) * m_step + m_min;
        val = std::clamp(val, m_min, m_max);
    }
    // Int 模式：始终取整
    if (m_type == SliderType::Int) {
        val = std::round(val);
    }
    m_value = val;
    // 平滑值同步跳跃（首次设置直接跟上，避免初始偏移）
    m_smoothValue = m_value;
}

void GUISlider::SetRange(double min, double max) {
    m_min = min;
    m_max = max;
    m_value = std::clamp(m_value, m_min, m_max);
    m_smoothValue = m_value;
}

// ============================================================================
// 滑块位置计算
// ============================================================================
float GUISlider::GetTrackOffset() const {
    return m_showInputBox ? m_inputBoxWidth + 6.0f : 0.0f;
}

float GUISlider::GetThumbPosition() const {
    // 使用平滑值计算位置，使 Int 滑块在步进之间平滑移动
    double renderVal = m_smoothValue;
    double ratio = (m_max > m_min) ? (renderVal - m_min) / (m_max - m_min) : 0.0;
    float offset = GetTrackOffset();
    if (m_vertical) {
        float trackHeight = m_height - m_thumbSize;
        return (float)(m_y + trackHeight * (1.0 - ratio));
    } else {
        float trackWidth = (m_width - offset) - m_thumbSize;
        return (float)(m_x + offset + trackWidth * ratio);
    }
}

// ============================================================================
// 碰撞检测（包含内嵌输入框 + 滑块和滑槽）
// ============================================================================
bool GUISlider::containsPoint(float px, float py) const {
    // 仅滑槽（track）区域可点击，标签/数值文字区域不可交互
    return IsPointOnTrack(px, py) || IsPointOnInputBox(px, py);
}

bool GUISlider::IsPointOnTrack(float px, float py) const {
    float halfThumb = m_thumbSize / 2.0f;
    float trackOffset = GetTrackOffset();

    if (m_vertical) {
        // 垂直滑槽：控件高度范围内的竖条带
        float cx = m_x + m_width / 2.0f;
        return px >= cx - halfThumb && px <= cx + halfThumb &&
               py >= m_y && py <= m_y + m_height;
    } else {
        // 水平滑槽：控件宽度范围内、thumb 高度的横条带
        float cy = m_y + m_height / 2.0f + 8;
        return px >= m_x + trackOffset && px <= m_x + m_width &&
               py >= cy - halfThumb && py <= cy + halfThumb;
    }
}

bool GUISlider::IsPointOnInputBox(float px, float py) const {
    if (!m_showInputBox) return false;
    return px >= m_x && px <= m_x + m_inputBoxWidth &&
           py >= m_y && py <= m_y + m_height;
}

// ============================================================================
// 绘制
// ============================================================================
void GUISlider::draw() {
    if (!m_visible) return;

    auto pos = getAbsolutePosition();
    float thumbPos = GetThumbPosition();
    float halfThumb = m_thumbSize / 2.0f;

    // === 滑槽（track） ===
    if (m_vertical) {
        // 垂直滑槽
        float trackX = pos.x + m_width / 2.0f - 3.0f;
        float trackY = pos.y + halfThumb;
        float trackW = 6.0f;
        float trackH = m_height - m_thumbSize;

        RLRectangle trackRect{ trackX, trackY, trackW, trackH };
        RLDrawRectangleRounded(trackRect, m_roundness, 8, m_trackColor);

        // 填充部分（从底部到滑块）
        float fillH = trackH - (thumbPos - trackY);
        if (fillH > 0) {
            RLRectangle fillRect{ trackX, thumbPos + halfThumb, trackW, fillH };
            RLDrawRectangleRounded(fillRect, m_roundness, 8, m_fillColor);
        }
    } else {
        // 水平滑槽
        float trackOffset = GetTrackOffset();
        float trackX = pos.x + trackOffset + halfThumb;
        float trackY = pos.y + m_height / 2.0f - 3.0f;
        float trackW = (m_width - trackOffset) - m_thumbSize;
        float trackH = 6.0f;

        RLRectangle trackRect{ trackX, trackY + 8, trackW, trackH };
        RLDrawRectangleRounded(trackRect, m_roundness, 8, m_trackColor);

        // 填充部分（从左侧到滑块）
        float fillW = thumbPos - trackX;
        if (fillW > 0) {
            RLRectangle fillRect{ trackX, trackY + 8, fillW, trackH };
            RLDrawRectangleRounded(fillRect, m_roundness, 8, m_fillColor);
        }
    }

    // === 滑块（thumb） ===
    RLColor thumbColor = m_thumbColor;
    if (m_dragState == DragState::Dragging) thumbColor = m_thumbDragColor;
    else if (m_dragState == DragState::Hovered) thumbColor = m_thumbHoverColor;

    if (m_vertical) {
        RLDrawCircle(thumbPos + halfThumb, pos.x + m_width / 2.0f, halfThumb, thumbColor);
        RLDrawCircleLines(thumbPos + halfThumb, pos.x + m_width / 2.0f, halfThumb,
                          RLColor{ 100, 100, 150, 255 });
    } else {
        RLDrawCircle(thumbPos + halfThumb, pos.y + m_height / 2.0f + 8, halfThumb, thumbColor);
        RLDrawCircleLines(thumbPos + halfThumb, pos.y + m_height / 2.0f + 8, halfThumb,
                          RLColor{ 100, 100, 150, 255 });
    }

    // === 内嵌输入框由 GUIManager 独立绘制（z-index 高于滑块）===

    // === 标签和数值显示 ===
    std::string displayStr;
    if (!m_label.empty()) {
        displayStr = m_label;
    }
    if (m_showValue) {
        char buf[32];
        if (m_type == SliderType::Int) {
            snprintf(buf, sizeof(buf), "%d", static_cast<int>(std::round(m_value)));
        } else if (m_value == static_cast<long long>(m_value)) {
            snprintf(buf, sizeof(buf), "%.0f", m_value);
        } else {
            snprintf(buf, sizeof(buf), "%.2f", m_value);
        }
        if (!displayStr.empty()) displayStr += ":";
        displayStr += buf;
    }

    if (!displayStr.empty()) {
        RLVector2 textSize = MeasureTextCustom(displayStr.c_str(), m_fontSize, 2);
        float tx, ty;
        if (m_vertical) {
            tx = pos.x + m_width + 8;
            ty = pos.y + (m_height - textSize.y) / 2.0f;
        } else {
            tx = pos.x + m_width + 8;
            ty = pos.y + (m_height - textSize.y) / 2.0f;
        } 
        DrawTextCustom(displayStr.c_str(), { tx - 115, ty - 8 }, m_fontSize, m_textColor, 2);
    }
}

// ============================================================================
// 鼠标 → 值更新
// ============================================================================
void GUISlider::UpdateFromMouse(float mx, float my) {
    double ratio;
    float offset = GetTrackOffset();
    if (m_vertical) {
        float trackHeight = m_height - m_thumbSize;
        float trackTop = m_y + (m_height - m_thumbSize);
        ratio = (trackTop - my) / trackHeight;
    } else {
        float trackWidth = (m_width - offset) - m_thumbSize;
        ratio = (mx - (m_x + offset + m_thumbSize / 2.0f)) / trackWidth;
    }

    ratio = std::clamp(ratio, 0.0, 1.0);
    double newVal = m_min + ratio * (m_max - m_min);

    if (m_useStep && m_step > 0) {
        newVal = std::round((newVal - m_min) / m_step) * m_step + m_min;
        newVal = std::clamp(newVal, m_min, m_max);
    }

    // Int 模式：取整
    if (m_type == SliderType::Int) {
        newVal = std::round(newVal);
    }

    if (newVal != m_value) {
        m_value = newVal;
        if (m_onChange) m_onChange(m_value);
    }
}

// ============================================================================
// 鼠标辅助检测：判断坐标是否在滑块圆形区域内
// ============================================================================
static bool IsPointOnThumb(float thumbPos, float thumbSize, bool vertical,
                           float widgetX, float widgetY, float widgetW, float widgetH,
                           float px, float py) {
    float half = thumbSize / 2.0f;
    float cx, cy;
    if (vertical) {
        // 垂直缩略图：圆心在 (widgetX + widgetW/2, thumbPos + half)
        cx = widgetX + widgetW / 2.0f;
        cy = thumbPos + half;
    } else {
        // 水平缩略图：绘制时有 +8 的 Y 偏移 (pos.y + m_height/2 + 8)
        cx = thumbPos + half;
        cy = widgetY + widgetH / 2.0f + 8;
    }
    float dx = px - cx;
    float dy = py - cy;
    return (dx * dx + dy * dy) <= half * half;
}

// ============================================================================
// 更新
// ============================================================================
void GUISlider::update(float dt) {
    if (!m_visible || !m_enabled) {
        m_dragState = DragState::None;
        return;
    }

    // PopupInput 状态下的子控件更新
    if (m_popupOpen && m_popupInput) {
        m_popupInput->update(dt);
        if (!m_popupInput->IsFocused()) {
            ClosePopupInput();
        }
        return;
    }

    // 内嵌输入框位置/值同步
    if (m_showInputBox && m_inputBox && m_inputBox->isVisible()) {
        auto absPos = getAbsolutePosition();
        m_inputBox->SetPosition(absPos.x, absPos.y);
        m_inputBox->SetSize(m_inputBoxWidth, m_height);
        m_inputBox->SetNumericRange(m_min, m_max);

        bool nowFocused = m_inputBox->IsFocused();
        if (nowFocused) {
            // 输入框聚焦中 → 不覆盖用户输入
        } else {
            // 输入框未聚焦 → 将 slider 值同步到输入框
            m_inputBox->SetNumericValue(m_value);
            // 失去焦点时提交（仅过渡瞬间）
            if (m_inputBoxPrevFocused) {
                double val = std::clamp(m_inputBox->GetNumericValue(), m_min, m_max);
                SetValue(val);
                m_inputBox->SetNumericValue(m_value);
                if (m_onChange) m_onChange(m_value);
                if (m_onSubmit) m_onSubmit(m_value);
            }
        }
        m_inputBoxPrevFocused = nowFocused;
    }

    // 平滑值插值追赶（滑块位置平滑动画）
    if (std::abs(m_smoothValue - m_value) > 0.001) {
        m_smoothValue += (m_value - m_smoothValue) * std::min(1.0f, m_smoothSpeed * dt);
    } else {
        m_smoothValue = m_value;
    }
}

// ============================================================================
// 鼠标事件回调（由 GUIManager 统一派发）
// ============================================================================
void GUISlider::onMouseEnter() {
    RLSetMouseCursor(RL_E_MOUSE_CURSOR_POINTING_HAND);
}

void GUISlider::SetShowInputBox(bool show) {
    if (show == m_showInputBox) return;
    m_showInputBox = show;
    m_inputBoxPrevFocused = false;

    if (show) {
        if (!m_inputBox) {
            int inputId = m_id + 1000000;
            m_inputBox = new TextBox(inputId, m_zIndex + 1, m_x, m_y, m_inputBoxWidth, m_height);
            m_inputBox->SetStyle(TextBoxStyle::Numeric);
            m_inputBox->SetNumericRange(m_min, m_max);
            m_inputBox->SetNumericFormat("%.2f");
            m_inputBox->SetNumericValue(m_value);
            m_inputBox->SetFont(m_fontSize, 1.0f, m_fontSize);
            m_inputBox->setVisible(true);
            // Enter 键确认 → 提交数值并让 slider 重新获得焦点
            m_inputBox->SetOnEnterCallback([this]() {
                double val = std::clamp(m_inputBox->GetNumericValue(), m_min, m_max);
                SetValue(val);
                m_inputBox->SetNumericValue(m_value);
                if (m_onChange) m_onChange(m_value);
                if (m_onSubmit) m_onSubmit(m_value);
                // 标记为已处理，防止下一帧 update 中的焦点丢失分支重复提交
                m_inputBoxPrevFocused = false;
                GUIManager::GetInstance().setFocus(m_id);
            });
            GUIManager::GetInstance().addWidget(m_inputBox);
        } else {
            m_inputBox->setVisible(true);
        }
    } else {
        if (m_inputBox) {
            m_inputBox->setVisible(false);
        }
    }
}

void GUISlider::onMouseLeave() {
    if (m_dragState != DragState::Dragging) {
        m_dragState = DragState::None;
        RLSetMouseCursor(RL_E_MOUSE_CURSOR_DEFAULT);
    }
}

void GUISlider::onMouseMove(const MouseState& state) {
    if (m_dragState == DragState::Dragging) {
        // 拖拽中持续更新值
        UpdateFromMouse(state.position.x, state.position.y);
        RLSetMouseCursor(RL_E_MOUSE_CURSOR_POINTING_HAND);
    } else {
        // 悬停时检测 thumb 高亮
        float thumbPos = GetThumbPosition();
        bool onThumb = IsPointOnThumb(thumbPos, m_thumbSize, m_vertical,
                                      m_x, m_y, m_width, m_height,
                                      state.position.x, state.position.y);
        m_dragState = onThumb ? DragState::Hovered : DragState::None;
        RLSetMouseCursor(RL_E_MOUSE_CURSOR_POINTING_HAND);
    }
}

void GUISlider::onMouseDown(const MouseState& state) {
    // 点击内嵌输入框区域 → 交给输入框处理，不触发拖拽
    if (IsPointOnInputBox(state.position.x, state.position.y)) return;

    // Ctrl+单击滑块 → 弹出精确输入
    bool ctrlHeld = RLIsKeyDown(RL_E_KEY_LEFT_CONTROL) || RLIsKeyDown(RL_E_KEY_RIGHT_CONTROL);
    float thumbPos = GetThumbPosition();
    bool onThumb = IsPointOnThumb(thumbPos, m_thumbSize, m_vertical,
                                  m_x, m_y, m_width, m_height,
                                  state.position.x, state.position.y);
    if (ctrlHeld && onThumb) {
        OpenPopupInput();
        return;
    }

    m_dragState = DragState::Dragging;
    RLSetMouseCursor(RL_E_MOUSE_CURSOR_POINTING_HAND);
    UpdateFromMouse(state.position.x, state.position.y);
}

void GUISlider::onMouseUp(const MouseState& state) {
    if (m_dragState == DragState::Dragging) {
        m_dragState = DragState::None;
        if (m_onSubmit) m_onSubmit(m_value);
    }
}

// ============================================================================
// 浮窗输入（Ctrl+单击滑块）
// ============================================================================
void GUISlider::OpenPopupInput() {
    if (!m_popupInput) {
        m_popupInput = new TextBox(999999, 9999, 0, 0, 120, 28);
        m_popupInput->SetStyle(TextBoxStyle::PopupInput);
        m_popupInput->SetNumericRange(m_min, m_max);
        m_popupInput->SetPopupCallback([this](double val) {
            SetValue(val);
            if (m_onChange) m_onChange(m_value);
            if (m_onSubmit) m_onSubmit(m_value);
            ClosePopupInput();
        });
        m_popupInput->SetFont(18.0f, 1.0f, 18.0f);
    }

    // Int 模式：弹出输入框也用整数格式
    if (m_type == SliderType::Int) {
        m_popupInput->SetNumericFormat("%.0f");
    } else {
        m_popupInput->SetNumericFormat("%.6g");
    }

    m_popupOpen = true;

    // 浮窗定位在滑块上方
    float thumbPos = GetThumbPosition();
    float popupX, popupY;
    if (m_vertical) {
        popupX = m_x + m_width + 8;
        popupY = thumbPos - 14;
    } else {
        popupX = thumbPos - 20;
        popupY = m_y - 32;
    }
    m_popupInput->SetPosition(popupX, popupY);
    m_popupInput->SetSize(120, 28);
    m_popupInput->SetNumericValue(m_value);
    m_popupInput->setVisible(true);

    GUIManager::GetInstance().addWidget(m_popupInput);
    GUIManager::GetInstance().setFocus(m_popupInput->getId());
}

void GUISlider::ClosePopupInput() {
    if (!m_popupOpen) return;
    m_popupOpen = false;
    if (m_popupInput) {
        m_popupInput->setVisible(false);
        GUIManager::GetInstance().removeWidget(m_popupInput->getId());
        m_popupInput = nullptr;  // 所有权已归还，指针失效
    }
    GUIManager::GetInstance().setFocus(-1);
}

// ============================================================================
// 焦点
// ============================================================================
void GUISlider::onFocus() {
}

void GUISlider::onBlur() {
    if (m_dragState == DragState::Dragging) {
        m_dragState = DragState::None;
    }
}
