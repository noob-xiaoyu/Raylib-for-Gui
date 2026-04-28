#include "gui_button.h"
#include "raylib.h"
#include "../backends/font/FontManager.h"

// ============================================================================
// GUIButton 实现（含 Fluent Design 动效）
//
// 从 core_fluent_button.c 移植的视觉特性：
//   - 指数平滑的 hover/press/focus 动画
//   - 软阴影（悬停浮现）
//   - Reveal 高光（鼠标跟随径向渐变）
//   - Ripple 涟漪（点击波纹扩散）
//   - Focus Ring（脉冲发光边框）
//   - 微缩放效果（hover 放大 1.5%, press 缩小 1%）
// ============================================================================

GUIButton::GUIButton(int id, int zIndex, const std::string& text, float x, float y,
                     float width, float height)
    : GUIWidget(id, zIndex)
    , m_text(text)
    , m_normalColor(RLColor{ 50, 50, 80, 255 })
    , m_hoverColor(RLColor{ 70, 70, 120, 255 })
    , m_pressColor(RLColor{ 30, 30, 50, 255 })
    , m_textColor(WHITE)
    , m_fontSize(20)
    , m_roundness(0.3f)
{
    setPosition(x, y);
    setSize(width, height);
}

void GUIButton::draw() {
    if (!m_visible) return;
    auto pos = getAbsolutePosition();

    // ---- 1. 计算动效因子 ----
    float h = Smoothstep(m_animHover);   // hover 平滑值
    float p = EaseOutCubic(m_animPress); // press 平滑值
    float f = Smoothstep(m_animFocus);   // focus 平滑值

    // ---- 2. 微缩放 ----
    float scale = 1.0f + (m_fluentEffects ? (0.015f * h - 0.010f * p) : 0.0f);
    RLRectangle rec;
    rec.x = pos.x + (m_width  - m_width  * scale) * 0.5f;
    rec.y = pos.y + (m_height - m_height * scale) * 0.5f + 1.0f * p;
    rec.width  = m_width  * scale;
    rec.height = m_height * scale;

    // ---- 3. 颜色混合 ----
    RLColor bgColor, borderColor, overlayColor;
    if (!m_enabled) {
        bgColor = m_normalColor;
        bgColor.a = (unsigned char)((float)bgColor.a * 0.6f);
        borderColor = RLColor{ 80, 80, 120, 40 };
        overlayColor = RLColor{ 0, 0, 0, 0 };
    } else {
        // 基础颜色由状态决定
        RLColor baseColor;
        switch (m_state) {
            case ButtonState::Normal:  baseColor = m_normalColor; break;
            case ButtonState::Hovered: baseColor = m_hoverColor;  break;
            case ButtonState::Pressed: baseColor = m_pressColor;  break;
        }
        bgColor = baseColor;

        // 悬停叠加高亮
        unsigned char overlayA = (unsigned char)(10.0f * h);
        overlayColor = RLColor{ 255, 255, 255, overlayA };

        // 边框颜色（悬停时更亮）
        unsigned char borderA = (unsigned char)(60 + 40 * h + 60 * f);
        borderColor = RLColor{ 120, 140, 200, borderA };
    }

    // 圆角半径（像素）-> raylib roundness
    float radiusPx = m_roundness * 10.0f;
    if (radiusPx < 2.0f) radiusPx = 2.0f;
    float roundVal = RoundnessForRadius(rec.width, rec.height, radiusPx);
    int seg = 10;

    // ---- 4. 软阴影 ----
    if (m_fluentEffects && m_enabled) {
        float elev = Clamp01(0.10f + 0.55f * h - 0.40f * p);
        if (elev > 0.01f) {
            DrawSoftShadowRounded(rec, radiusPx, elev,
                RLColor{ 0, 0, 0, 255 });
        }
    }

    // ---- 5. 填充 ----
    RLDrawRectangleRounded(rec, roundVal, seg, bgColor);
    if (overlayColor.a > 0) {
        RLDrawRectangleRounded(rec, roundVal, seg, overlayColor);
    }
    RLDrawRectangleRoundedLinesEx(rec, roundVal, seg, 1.0f, borderColor);

    // ---- 6. Reveal 高光 ----
    if (m_fluentEffects && m_enabled) {
        float revealIntensity = Clamp01(h * 0.9f + f * 0.25f);
        DrawRevealGradientClipped(rec, m_mousePos, revealIntensity);
    }

    // ---- 7. Ripple 涟漪 ----
    if (m_fluentEffects && m_enabled && (m_rippleActive || m_rippleT > 0.0f)) {
        float maxRad = sqrtf(rec.width * rec.width + rec.height * rec.height);
        DrawRippleEffect(rec, m_ripplePos, m_rippleT, maxRad,
                         RLColor{ 200, 220, 255, 80 });
    }

    // ---- 8. Focus Ring ----
    if (m_fluentEffects && m_enabled && f > 0.001f) {
        RLRectangle focusRec = rec;
        focusRec.x -= 2; focusRec.y -= 2;
        focusRec.width  += 4; focusRec.height += 4;
        RLColor ringColor = RLColor{ 180, 200, 255, 0 };
        ringColor.a = (unsigned char)(160.0f * f);
        RLDrawRectangleRoundedLinesEx(focusRec,
            RoundnessForRadius(focusRec.width, focusRec.height, radiusPx + 2.0f),
            seg, 2.0f, ringColor);
    }

    // ---- 9. 文本 ----
    RLVector2 textSize = MeasureTextCustom(m_text.c_str(), m_fontSize, 2);
    float tx = rec.x + (rec.width  - textSize.x) / 2.0f;
    float ty = rec.y + (rec.height - textSize.y) / 2.0f;
    DrawTextCustom(m_text.c_str(), { tx, ty }, m_fontSize, m_textColor, 2);
}

void GUIButton::update(float dt) {
    if (!m_visible || !m_enabled) {
        m_state = ButtonState::Normal;
    }

    // ---- Fluent 动效平滑更新 ----
    if (m_fluentEffects) {
        bool hot   = (m_state == ButtonState::Hovered || m_state == ButtonState::Pressed);
        bool down  = (m_state == ButtonState::Pressed);

        float hoverTarget = (m_enabled && hot) ? 1.0f : 0.0f;
        float pressTarget = down ? 1.0f : 0.0f;
        float focusTarget = m_focused ? 1.0f : 0.0f;

        m_animHover = ApproachExp(m_animHover, hoverTarget, 14.0f, dt);
        m_animPress = ApproachExp(m_animPress, pressTarget, 22.0f, dt);
        m_animFocus = ApproachExp(m_animFocus, focusTarget, 18.0f, dt);

        // Ripple 进度
        if (m_rippleActive) {
            m_rippleT += dt / 0.28f;
            if (m_rippleT >= 1.0f) {
                m_rippleT = 1.0f;
                m_rippleActive = false;
            }
        }
    } else {
        // 无动效时直接跳转
        m_animHover = (m_enabled && (m_state == ButtonState::Hovered || m_state == ButtonState::Pressed)) ? 1.0f : 0.0f;
        m_animPress = (m_state == ButtonState::Pressed) ? 1.0f : 0.0f;
        m_animFocus = m_focused ? 1.0f : 0.0f;
    }
}

// ---- 鼠标事件回调 ----
void GUIButton::onMouseEnter() {
    if (m_visible && m_enabled)
        m_state = ButtonState::Hovered;
}

void GUIButton::onMouseLeave() {
    m_state = ButtonState::Normal;
}

void GUIButton::onMouseDown(const MouseState& state) {
    if (m_visible && m_enabled) {
        m_state = ButtonState::Pressed;

        // 触发 Ripple
        if (m_fluentEffects) {
            m_ripplePos = state.position;
            m_rippleActive = true;
            m_rippleT = 0.0f;
        }
    }
}

void GUIButton::onMouseUp(const MouseState& state) {
    if (m_visible && m_enabled)
        m_state = ButtonState::Hovered;
}

void GUIButton::onMouseMove(const MouseState& state) {
    m_mousePos = state.position;
}

void GUIButton::onClick(const MouseState& state) {
    if (m_visible && m_enabled && m_onClick) {
        m_onClick();
    }
}

// ---- Setter ----
void GUIButton::setText(const std::string& text) {
    m_text = text;
}

void GUIButton::setOnClick(ClickCallback callback) {
    m_onClick = callback;
}

void GUIButton::setNormalColor(RLColor color) {
    m_normalColor = color;
}

void GUIButton::setHoverColor(RLColor color) {
    m_hoverColor = color;
}

void GUIButton::setPressColor(RLColor color) {
    m_pressColor = color;
}

void GUIButton::setTextColor(RLColor color) {
    m_textColor = color;
}

void GUIButton::setFontSize(int fontSize) {
    m_fontSize = fontSize;
}

void GUIButton::setRoundness(float roundness) {
    m_roundness = roundness;
}

void GUIButton::click() {
    if (m_onClick) {
        m_onClick();
    }
}

void GUIButton::onFocus() {
    // 由 update() 中的 m_animFocus 处理
}

void GUIButton::onBlur() {
    m_state = ButtonState::Normal;
}
