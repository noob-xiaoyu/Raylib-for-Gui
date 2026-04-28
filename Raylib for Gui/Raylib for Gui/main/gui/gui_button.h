#ifndef GUI_BUTTON_H
#define GUI_BUTTON_H

#include "../backends/gui/gui_widget.h"
#include "../backends/effects/VisualEffects.h"
#include <string>
#include <functional>

// ============================================================================
// GUIButton - 按钮控件（支持 Fluent Design 动效）
//
// 视觉特性（从 core_fluent_button.c 移植）：
//   - 悬停/按下/聚焦 指数平滑动画
//   - Reveal 高光（鼠标跟随径向渐变）
//   - Ripple 涟漪（点击波纹扩散）
//   - 软阴影（悬停时浮现）
//   - Focus Ring（脉冲发光边框）
// ============================================================================
class GUIButton : public GUIWidget {
public:
    using ClickCallback = std::function<void()>;

    GUIButton(int id, int zIndex, const std::string& text, float x, float y,
              float width, float height);

    void draw() override;
    void update(float dt) override;
    bool canBeFocused() const override { return true; }
    void onFocus() override;
    void onBlur() override;

    // 鼠标事件回调（由 GUIManager 统一派发）
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseDown(const MouseState& state) override;
    void onMouseUp(const MouseState& state) override;
    void onClick(const MouseState& state) override;
    void onMouseMove(const MouseState& state) override;

    void setText(const std::string& text);
    void setOnClick(ClickCallback callback);
    void setNormalColor(RLColor color);
    void setHoverColor(RLColor color);
    void setPressColor(RLColor color);
    void setTextColor(RLColor color);
    void setFontSize(int fontSize);
    void setRoundness(float roundness);

    // Fluent 动效开关
    void setFluentEffects(bool enabled) { m_fluentEffects = enabled; }
    bool getFluentEffects() const { return m_fluentEffects; }

    std::string getText() const { return m_text; }
    void click();

private:
    enum class ButtonState { Normal, Hovered, Pressed };
    ButtonState m_state = ButtonState::Normal;

    std::string m_text;
    ClickCallback m_onClick;
    RLColor m_normalColor;
    RLColor m_hoverColor;
    RLColor m_pressColor;
    RLColor m_textColor;
    int  m_fontSize;
    float m_roundness;

    // ---- Fluent 动效状态 ----
    bool m_fluentEffects = true;

    // 平滑动画因子（0..1，由 ApproachExp 驱动）
    float m_animHover   = 0.0f;
    float m_animPress   = 0.0f;
    float m_animFocus   = 0.0f;

    // Ripple 状态
    float   m_rippleT       = 0.0f;    // 0..1
    RLVector2 m_ripplePos   = { 0, 0 };
    bool    m_rippleActive   = false;

    // 鼠标位置缓存（用于 Reveal 高光）
    RLVector2 m_mousePos = { 0, 0 };
};

#endif // GUI_BUTTON_H
