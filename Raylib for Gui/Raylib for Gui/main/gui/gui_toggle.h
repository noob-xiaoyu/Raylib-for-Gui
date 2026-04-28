#ifndef GUI_TOGGLE_H
#define GUI_TOGGLE_H

#include "../backends/gui/gui_widget.h"
#include <string>
#include <functional>

// ============================================================================
// GUIToggle - 开关/切换控件
//
// 功能：
//   - 开/关两种状态，带滑动动画
//   - 支持标签显示
//   - 鼠标点击/键盘切换
//   - 状态变化回调
// ============================================================================
class GUIToggle : public GUIWidget {
public:
    using ToggleCallback = std::function<void(bool)>;

    GUIToggle(int id, int zIndex, const std::string& label, float x, float y,
              bool initialState = false);

    void draw() override;
    void update(float dt) override;
    bool canBeFocused() const override { return true; }
    void onFocus() override;
    void onBlur() override;

    bool containsPoint(float px, float py) const override;

    // 鼠标事件回调（由 GUIManager 统一派发）
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onClick(const MouseState& state) override;

    // 状态操作
    void setChecked(bool checked);
    bool isChecked() const { return m_checked; }
    void toggle();

    // 外观设置
    void setLabel(const std::string& label);
    void setOnToggle(ToggleCallback callback);
    void setTrackOnColor(RLColor color);
    void setTrackOffColor(RLColor color);
    void setThumbOnColor(RLColor color);
    void setThumbOffColor(RLColor color);
    void setThumbHoverColor(RLColor color);
    void setLabelColor(RLColor color);
    void setLabelSize(int size);
    void setTrackWidth(float width);
    void setTrackHeight(float height);

    std::string getLabel() const { return m_label; }
    RLRectangle getFocusRect() const override;

private:
    // 开关核心
    bool m_checked;
    bool m_prevChecked;

    // 动画插值 (0.0 = off, 1.0 = on)
    float m_animT = 0.0f;

    // 状态
    bool m_hovered = false;

    // 回调
    ToggleCallback m_onToggle;

    // 外观
    std::string m_label;
    RLColor m_trackOnColor;
    RLColor m_trackOffColor;
    RLColor m_thumbOnColor;
    RLColor m_thumbOffColor;
    RLColor m_thumbHoverColor;
    RLColor m_labelColor;
    int  m_labelSize;
    float m_trackWidth;
    float m_trackHeight;
    float m_thumbRadius;
    float m_roundness;

    // 布局常量
    static constexpr float k_thumbPadding = 3.0f;
    static constexpr float k_labelGap = 10.0f;
    static constexpr float k_animSpeed = 8.0f; // 过渡动画速度
};

#endif // GUI_TOGGLE_H
