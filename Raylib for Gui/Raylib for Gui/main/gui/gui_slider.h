#ifndef GUI_SLIDER_H
#define GUI_SLIDER_H

#include "../backends/gui/gui_widget.h"
#include "gui_TextBox.h"
#include <string>
#include <functional>

// ============================================================================
// SliderType - 滑块数值类型
// ============================================================================
enum class SliderType {
    Float,  // 浮点滑块（默认）
    Int     // 整数滑块，值始终取整
};

// ============================================================================
// GUISlider - 滑块控件
// 支持：水平/垂直方向、范围设定、数值显示、Ctrl+单击弹出精确输入
// ============================================================================
class GUISlider : public GUIWidget {
public:
    using ValueCallback = std::function<void(double)>;

    GUISlider(int id, int zIndex, float x, float y, float width, float height,
              SliderType type = SliderType::Float);
    ~GUISlider();

    void draw() override;
    void update(float dt) override;
    bool canBeFocused() const override { return true; }
    void onFocus() override;
    void onBlur() override;
    bool containsPoint(float px, float py) const override;

    // 鼠标事件回调（由 GUIManager 统一派发）
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseMove(const MouseState& state) override;
    void onMouseDown(const MouseState& state) override;
    void onMouseUp(const MouseState& state) override;

    // 值
    void SetValue(double val);
    double GetValue() const { return m_value; }

    // 范围
    void SetRange(double min, double max);
    double GetMin() const { return m_min; }
    double GetMax() const { return m_max; }

    // 步进
    void SetStep(double step) { m_step = step; m_useStep = true; }
    void SetContinuous(bool continuous) { m_useStep = !continuous; }
    double GetStep() const { return m_step; }

    // 方向
    void SetVertical(bool vertical) { m_vertical = vertical; }
    bool IsVertical() const { return m_vertical; }

    // 类型
    SliderType GetType() const { return m_type; }

    // 回调
    void SetOnChange(ValueCallback callback) { m_onChange = callback; }
    void SetOnSubmit(ValueCallback callback) { m_onSubmit = callback; }

    // 外观
    void SetTrackColor(RLColor color) { m_trackColor = color; }
    void SetFillColor(RLColor color) { m_fillColor = color; }
    void SetThumbColor(RLColor color) { m_thumbColor = color; }
    void SetThumbHoverColor(RLColor color) { m_thumbHoverColor = color; }
    void SetThumbDragColor(RLColor color) { m_thumbDragColor = color; }
    void SetTextColor(RLColor color) { m_textColor = color; }
    void SetLabel(const std::string& label) { m_label = label; }
    void SetShowValue(bool show) { m_showValue = show; }
    void SetFontSize(int size) { m_fontSize = size; }
    void SetRoundness(float roundness) { m_roundness = roundness; }

    // 获取滑块位置（用于绘制）
    float GetThumbPosition() const;
    float GetThumbSize() const { return m_thumbSize; }

    void SetThumbSize(float size) { m_thumbSize = size; }

    // 内嵌数值输入框（可选，在滑块左侧）
    void SetShowInputBox(bool show);
    bool HasInputBox() const { return m_showInputBox; }
    void SetInputBoxWidth(float width) { m_inputBoxWidth = width; }
    float GetInputBoxWidth() const { return m_inputBoxWidth; }
    TextBox* GetInputBox() const { return m_inputBox; }

private:
    void UpdateFromMouse(float mx, float my);
    void OpenPopupInput();
    void ClosePopupInput();
    float GetTrackOffset() const;
    bool IsPointOnTrack(float px, float py) const;
    bool IsPointOnInputBox(float px, float py) const;

    // 数值类型
    SliderType m_type = SliderType::Float;

    // 值（实际值通过 min/max 映射）
    double m_value = 0.0;
    double m_smoothValue = 0.0; // 平滑渲染值，插值追赶 m_value
    float m_smoothSpeed = 12.0f; // 插值速度

    double m_min = 0.0;
    double m_max = 100.0;
    double m_step = 1.0;
    bool m_useStep = true;

    // 方向
    bool m_vertical = false;

    // 状态
    enum class DragState { None, Hovered, Dragging };
    DragState m_dragState = DragState::None;

    // 尺寸
    float m_thumbSize = 16.0f;

    // 颜色
    RLColor m_trackColor{ 50, 50, 70, 255 };
    RLColor m_fillColor{ 100, 150, 255, 255 };
    RLColor m_thumbColor{ 180, 180, 200, 255 };
    RLColor m_thumbHoverColor{ 200, 200, 220, 255 };
    RLColor m_thumbDragColor{ 220, 220, 255, 255 };
    RLColor m_textColor{ 200, 200, 220, 255 };

    // 标签/数值显示
    std::string m_label;
    bool m_showValue = true;
    int m_fontSize = 16;
    float m_roundness = 0.3f;

    // 内嵌数值输入框
    bool m_showInputBox = false;
    float m_inputBoxWidth = 60.0f;
    TextBox* m_inputBox = nullptr;
    bool m_inputBoxPrevFocused = false;

    // 浮窗输入
    TextBox* m_popupInput = nullptr;
    bool m_popupOpen = false;
    ValueCallback m_onChange;
    ValueCallback m_onSubmit;
};

#endif // GUI_SLIDER_H
