#pragma once
#include <raylib.h>

// MouseState 在 gui_widget.h 中定义，此处仅用于引用参数
struct MouseState;

// ============================================================================
// ScrollBar - 通用滚动条组件（支持垂直和水平方向）
// 支持鼠标滚轮、点击滑块拖拽、点击轨道翻页
// TextBox 和下拉框共享使用
// ============================================================================

enum class ScrollBarOrientation {
    Vertical,
    Horizontal
};
class ScrollBar {
public:
    ScrollBar();

    // 设置滚动条区域
    void setRect(float x, float y, float width, float height);

    // 设置内容与视口参数
    void setContent(float contentSize, float viewportSize);

    // 处理事件（鼠标点击、拖拽）
    // 由各个控件的 update() 调用，传入同一帧 MouseState 快照
    // 注意：滚轮由控件自身的 onMouseWheel 回调处理，此处不再重复
    void handleScroll(const MouseState& ms);

    // 绘制滚动条
    void draw() const;

    // 获取/设置滚动偏移（单位：像素，负值表示向上滚动）
    float getScrollOffset() const { return m_scrollOffset; }
    void setScrollOffset(float offset) { m_scrollOffset = offset; }

    // 是否正在拖拽
    bool isDragging() const { return m_isDragging; }

    // 是否需要显示滚动条
    bool isVisible() const { return m_visible; }

    // 获取滚动条矩形区域
    RLRectangle getRect() const { return { m_x, m_y, m_width, m_height }; }

    // 获取滚动进度 [0,1]
    float getProgress() const;

    // 样式设置
    void setThickness(float thickness) { m_thickness = thickness; }
    void setMinThumbSize(float size) { m_minThumbSize = size; }

    // 方向设置（默认垂直）
    void setOrientation(ScrollBarOrientation orientation) { m_orientation = orientation; }
    ScrollBarOrientation getOrientation() const { return m_orientation; }

private:
    void updateThumb();

    float m_x, m_y, m_width, m_height;
    float m_contentSize;
    float m_viewportSize;
    float m_scrollOffset;
    float m_maxScroll;

    float m_thickness;
    float m_minThumbSize;

    // 滑块
    float m_thumbY;
    float m_thumbHeight;
    float m_trackHeight;

    bool m_visible;
    bool m_isDragging;
    float m_dragOffset;   // 拖拽时鼠标相对滑块顶部的偏移

    ScrollBarOrientation m_orientation;

    RLColor m_trackColor;
    RLColor m_thumbColor;
    RLColor m_thumbHoverColor;
    RLColor m_thumbDragColor;
};
