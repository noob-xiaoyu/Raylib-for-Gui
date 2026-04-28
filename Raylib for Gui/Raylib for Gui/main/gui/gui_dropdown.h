#ifndef GUI_DROPDOWN_H
#define GUI_DROPDOWN_H

#include "../backends/gui/gui_widget.h"
#include "../backends/gui/gui_scrollbar.h"
#include <string>
#include <vector>
#include <functional>

// ============================================================================
// GUIDropdown - 下拉框控件
// 点击展开选项列表，点击选项后收起并触发回调
// ============================================================================
class GUIDropdown : public GUIWidget {
public:
    using SelectCallback = std::function<void(int index, const std::string& text)>;

    GUIDropdown(int id, int zIndex, float x, float y, float width, float height);

    void draw() override;
    void update(float dt) override;
    bool containsPoint(float px, float py) const override;
    bool canBeFocused() const override { return true; }
    void onFocus() override;
    void onBlur() override;

    // 鼠标事件回调
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseWheel(const MouseState& state) override;

    // 选项管理
    void addItem(const std::string& item);
    void removeItem(int index);
    void clearItems();
    void setItems(const std::vector<std::string>& items);

    // 选中
    void setSelectedIndex(int index);
    int getSelectedIndex() const { return m_selectedIndex; }
    std::string getSelectedText() const;

    // 外观设置
    void setTextColor(RLColor color);
    void setNormalColor(RLColor color);
    void setHoverColor(RLColor color);
    void setExpandedColor(RLColor color);
    void setItemHoverColor(RLColor color);
    void setFontSize(int fontSize);
    void setRoundness(float roundness);
    void setMaxVisibleItems(int count);
    int getMaxVisibleItems() const { return m_maxVisibleItems; }
    int getItemCount() const { return static_cast<int>(m_items.size()); }

    // 回调
    void setOnSelect(SelectCallback callback);

    // 展开/收起
    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool expanded);
    float getAnimT() const { return m_animT; }

private:
    void drawButton();
    void drawDropdownList();
    int  getHoveredItemIndex() const;

    std::vector<std::string> m_items;
    int m_selectedIndex = -1;
    bool m_expanded = false;

    // 颜色
    RLColor m_textColor;
    RLColor m_normalColor;
    RLColor m_hoverColor;
    RLColor m_expandedColor;
    RLColor m_itemHoverColor;

    int  m_fontSize;
    float m_roundness;
    int  m_maxVisibleItems;
    int  m_scrollOffset = 0;  // 滚动偏移（首个可见项的索引）
    ScrollBar m_scrollbar;

    bool m_mainButtonHovered = false;  // 由 onMouseEnter/Leave 维护

    // 展开/收起平滑动画
    float m_animT = 0.0f;       // 0=收起, 1=完全展开
    float m_animTarget = 0.0f;  // 动画目标值
    float m_animSpeed = 10.0f;

    SelectCallback m_onSelect;
};

#endif // GUI_DROPDOWN_H
