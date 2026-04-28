#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H

#include "gui_widget.h"
#include "gui_panel.h"
#include "../z-index/z_index.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <algorithm>
#include <type_traits>

// ============================================================================
// GUIManager - 基于 z-index 排序的 GUI 管理器（单例）
//
// 核心职责：
//   - 控件生命周期管理（创建、移除、查询）
//   - 鼠标/键盘事件分发（含焦点导航）
//   - 按 z-index 顺序渲染
//   - 下拉框展开自动置顶
//
// 设计要点：
//   - ZIndexManager 作为排序的唯一权威来源，摒弃冗余的 m_drawOrder
//   - 渲染时直接从 ZIndexManager 获取排序结果，逐控件调用 draw()
//   - 点击消耗机制供 dropdown 等控件阻止事件穿透
// ============================================================================
class GUIManager {
public:
    static GUIManager& GetInstance();

    // ---------- 控件创建 ----------
    // 模板创建：自动注册到 z-index 系统、焦点列表
    template<typename T, typename... Args>
    T* createWidget(int id, int zIndex, Args&&... args) {
        static_assert(std::is_base_of<GUIWidget, T>::value,
                      "T must inherit from GUIWidget");

        auto widget = std::make_unique<T>(id, zIndex, std::forward<Args>(args)...);
        T* ptr = widget.get();
        m_widgets[id] = std::move(widget);

        // 注册到 z-index 系统（仅排序，不设绘制回调）
        m_zIndex.addElement(id, zIndex);

        if (ptr->canBeFocused()) {
            m_focusableOrder.push_back(id);
        }
        return ptr;
    }

    void removeWidget(int id);
    GUIWidget* getWidget(int id) const;

    // 添加已创建好的控件（用于动态创建的弹窗控件等）
    void addWidget(GUIWidget* widget);

    // ---------- 事件处理 ----------
    void handleEvents();
    void handleMouseInput();
    void handleKeyboardInput();

    // 暴露鼠标状态快照，供控件在 update() 中读取（可选降级方案）
    const MouseState& getMouseState() const { return m_mouseState; }

    // ---------- 焦点管理 ----------
    void setFocus(int id);
    void clearFocus();
    void focusNext();
    void focusPrevious();
    int  getFocusedId() const { return m_focusedId; }
    GUIWidget* getFocusedWidget() const;

    // ---------- 点击消耗 ----------
    // 供 dropdown 等控件调用：当展开列表已处理点击后，
    // 阻止同帧内其他底层控件重复处理
    void consumeClick() { m_clickConsumed = true; }
    bool isClickConsumed() const { return m_clickConsumed; }
    void resetClickConsumed() { m_clickConsumed = false; }

    // ---------- 渲染 ----------
    void render();
    void renderDebug();

    // ---------- Z-Index 操作 ----------
    void bringToFront(int id);
    void sendToBack(int id);
    void moveUp(int id);
    void moveDown(int id);
    void setZIndex(int id, int newZ);

    // ---------- 布局工具 ----------
    void centerWidget(int id, float parentW, float parentH);

    // ---------- 热更新布局 ----------
    // 对所有设置了锚点的控件应用布局重算（窗口缩放后调用）
    void applyAllLayout(float parentW, float parentH);

    // ---------- 展开下拉框检测 ----------
    bool isPointOnExpandedDropdown(float px, float py) const;
    bool hasExpandedDropdown() const { return m_expandedDropdownId != -1; }

private:
    GUIManager() = default;
    ~GUIManager() = default;
    GUIManager(const GUIManager&) = delete;
    GUIManager& operator=(const GUIManager&) = delete;

    // 将 ZIndexManager 中该控件的当前层级同步回控件自身
    void syncWidgetZIndex(int id);

    // 绘制焦点指示器（脉冲发光边框）
    void drawWidgetFocus(int id);

    // 焦点框平滑动画
    RLRectangle m_focusAnimRect = { 0, 0, 0, 0 };
    RLRectangle m_focusTargetRect = { 0, 0, 0, 0 };
    float m_focusAnimSpeed = 12.0f;
    float m_focusAlpha = 0.0f; // 焦点框透明度（0~1），失焦淡出

    ZIndexManager m_zIndex;
    std::unordered_map<int, std::unique_ptr<GUIWidget>> m_widgets;

    // ---------- 鼠标输入系统 ----------
    MouseState m_mouseState = {};        // 每帧鼠标完整快照
    int m_prevHoveredId = -1;           // 前一帧悬停，用于 enter/leave 检测

    // 可聚焦控件的 id 列表（Tab 导航用）
    std::vector<int> m_focusableOrder;
    int m_focusedId = -1;
    int m_hoveredId = -1;
    int m_pressedId = -1;

    // 下拉框展开置顶追踪
    int m_expandedDropdownId = -1;
    int m_savedDropdownZIndex = 0;

    bool m_clickConsumed = false;
};

// 便捷全局函数（供 dropdown/multidropdown 在 update() 中调用）
inline void ConsumeClick() { GUIManager::GetInstance().consumeClick(); }
inline bool IsClickConsumed() { return GUIManager::GetInstance().isClickConsumed(); }

#endif // GUI_MANAGER_H
