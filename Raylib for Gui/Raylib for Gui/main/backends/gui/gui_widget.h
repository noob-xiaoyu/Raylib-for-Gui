#ifndef GUI_WIDGET_H
#define GUI_WIDGET_H

#include "raylib.h"
#include <string>

// ============================================================================
// MouseState - 每帧鼠标完整快照
// ============================================================================
struct MouseState {
    RLVector2 position;       // 当前位置（像素）
    RLVector2 delta;          // 相对于上一帧的位移
    float     wheelMove;      // 垂直滚轮增量
    float     wheelMoveH;     // 水平滚轮增量（左右倾斜/触摸板横向滑动）
    bool      leftPressed;    // 本帧按下
    bool      leftReleased;   // 本帧释放
    bool      leftDown;       // 当前按住
    bool      rightPressed;
    bool      rightReleased;
    bool      rightDown;
};

// ============================================================================
// LayoutAnchor - 布局锚点标志
// 用于热更新位置大小：窗口缩放时控件自动调整位置和尺寸
// ============================================================================
enum LayoutAnchor : int {
    Layout_None       = 0,
    Layout_Left       = 1 << 0,      // 保持距左边缘距离
    Layout_Top        = 1 << 1,      // 保持距上边缘距离
    Layout_Right      = 1 << 2,      // 保持距右边缘距离
    Layout_Bottom     = 1 << 3,      // 保持距下边缘距离
    // 便捷组合
    Layout_TopLeft    = Layout_Left | Layout_Top,
    Layout_TopRight   = Layout_Right | Layout_Top,
    Layout_BottomLeft = Layout_Left | Layout_Bottom,
    Layout_BottomRight= Layout_Right | Layout_Bottom,
    Layout_StretchH   = Layout_Left | Layout_Right,   // 水平拉伸
    Layout_StretchV   = Layout_Top  | Layout_Bottom,  // 垂直拉伸
    Layout_StretchAll = Layout_Left | Layout_Top | Layout_Right | Layout_Bottom,
};

// ============================================================================
// LayoutData - 每个控件的布局快照与锚点设置
// ============================================================================
struct LayoutData {
    float parentW = 0;        // 设置布局时的父容器宽度
    float parentH = 0;        // 设置布局时的父容器高度
    float initX = 0;          // 设置布局时的 x
    float initY = 0;          // 设置布局时的 y
    float initW = 0;          // 设置布局时的 width
    float initH = 0;          // 设置布局时的 height
    float marginL = 0;        // 距左边缘距离（Left 锚点时）
    float marginT = 0;        // 距上边缘距离（Top 锚点时）
    float marginR = 0;        // 距右边缘距离（Right 锚点时）
    float marginB = 0;        // 距下边缘距离（Bottom 锚点时）
    int   anchors = Layout_TopLeft;  // 锚点标志（默认左上角固定）
};

// ============================================================================
// GUIWidget - 所有 GUI 控件的基类
// 负责位置、大小、可见性、父子关系、布局锚点
// ============================================================================
class GUIWidget {
public:
    GUIWidget(int id, int zIndex = 0);
    virtual ~GUIWidget() = default;

    // ---- 核心虚函数 ----
    virtual void draw() = 0;
    virtual void update(float dt) {}

    // ---- 焦点事件 ----
    virtual void onFocus() {}
    virtual void onBlur() {}

    // ---- 鼠标事件回调（默认空实现，子类选择性重写） ----
    // 由 GUIManager 统一检测后调用，控件不再自行查询 raylib
    virtual void onMouseEnter() {}                              // 鼠标进入区域
    virtual void onMouseLeave() {}                              // 鼠标离开区域
    virtual void onMouseMove(const MouseState& state) {}        // 区域内移动
    virtual void onMouseDown(const MouseState& state) {}        // 区域内按下
    virtual void onMouseUp(const MouseState& state) {}          // 区域内释放
    virtual void onClick(const MouseState& state) {}            // 区域内按下+释放 = 单击
    virtual void onMouseWheel(const MouseState& state) {}       // 区域内滚轮

    // 碰撞检测（世界坐标）
    virtual bool containsPoint(float px, float py) const;

    // 获取绝对位置（考虑父容器偏移）
    RLVector2 getAbsolutePosition() const;

    // ---- 位置/大小设置（虚函数，子类可拦截） ----
    virtual void setPosition(float x, float y);
    virtual void setSize(float w, float h);

    void setVisible(bool visible);
    void setEnabled(bool enabled);
    void setZIndex(int zIndex);
    void setParent(GUIWidget* parent);
    void setColor(RLColor color);
    void setTag(const std::string& tag);

    int getId() const { return m_id; }
    int getZIndex() const { return m_zIndex; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }
    bool isVisible() const { return m_visible; }
    bool isEnabled() const { return m_enabled; }
    RLColor getColor() const { return m_color; }
    std::string getTag() const { return m_tag; }
    GUIWidget* getParent() const { return m_parent; }

    // ---- 布局锚点系统（热更新位置大小） ----
    // 设置锚点并记录当前布局快照
    void setAnchors(int anchorFlags, float parentW = 0, float parentH = 0);
    // 根据当前锚点和父容器新尺寸重新计算位置/大小
    void applyLayout(float parentW, float parentH);
    // 获取布局数据（供外部调整锚点边距等）
    LayoutData& getLayoutData() { return m_layout; }
    const LayoutData& getLayoutData() const { return m_layout; }
    // 保持原始大小（仅更新位置不改变尺寸，适用于标签等自定大小控件）
    void setKeepSize(bool keep) { m_keepSize = keep; }
    bool getKeepSize() const { return m_keepSize; }

    // 焦点
    virtual bool canBeFocused() const { return false; }
    void setFocused(bool focused);
    bool isFocused() const { return m_focused; }

    // 区域
    RLRectangle getBounds() const;

    // 焦点高亮框区域（默认等于 getBounds()，子类可重写缩小范围）
    virtual RLRectangle getFocusRect() const;


protected:
    int     m_id;
    int     m_zIndex;
    float   m_x, m_y;
    float   m_width, m_height;
    bool    m_visible;
    bool    m_enabled;
    bool    m_focused = false;
    RLColor m_color;
    std::string m_tag;
    GUIWidget*  m_parent = nullptr;
    LayoutData  m_layout;       // 布局锚点数据
    bool        m_keepSize = false;  // 保持原始大小（标签等自定大小控件）
};

#endif // GUI_WIDGET_H
