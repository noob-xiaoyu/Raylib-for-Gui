#include "gui_multidropdown.h"

#include "raylib.h"
#include <algorithm>
#include <sstream>
#include "../backends/gui/gui_widget.h"
#include "../backends/font/FontManager.h"
#include "../backends/gui/gui_manager.h"

// ============================================================================
// GUIMultiDropdown 实现
// ============================================================================
GUIMultiDropdown::GUIMultiDropdown(int id, int zIndex, float x, float y, float width, float height)
    : GUIWidget(id, zIndex)
    , m_textColor(WHITE)
    , m_normalColor(RLColor{ 50, 50, 80, 255 })
    , m_hoverColor(RLColor{ 70, 70, 120, 255 })
    , m_expandedColor(RLColor{ 60, 60, 100, 255 })
    , m_itemHoverColor(RLColor{ 90, 90, 150, 255 })
    , m_fontSize(20)
    , m_roundness(0.3f)
    , m_maxVisibleItems(6)
{
    setPosition(x, y);
    setSize(width, height);
}

// ============================================================================
// 选项管理
// ============================================================================
void GUIMultiDropdown::addItem(const std::string& item) {
    m_items.push_back(item);
    m_selected.push_back(false);
}

void GUIMultiDropdown::removeItem(int index) {
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        m_items.erase(m_items.begin() + index);
        m_selected.erase(m_selected.begin() + index);
    }
}

void GUIMultiDropdown::clearItems() {
    m_items.clear();
    m_selected.clear();
}

void GUIMultiDropdown::setItems(const std::vector<std::string>& items) {
    m_items = items;
    m_selected.assign(items.size(), false);
}

// ============================================================================
// 选中
// ============================================================================
void GUIMultiDropdown::setSelected(int index, bool selected) {
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        m_selected[index] = selected;
    }
}

bool GUIMultiDropdown::isSelected(int index) const {
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        return m_selected[index];
    }
    return false;
}

void GUIMultiDropdown::selectAll() {
    std::fill(m_selected.begin(), m_selected.end(), true);
}

void GUIMultiDropdown::deselectAll() {
    std::fill(m_selected.begin(), m_selected.end(), false);
}

std::vector<int> GUIMultiDropdown::getSelectedIndices() const {
    std::vector<int> result;
    for (size_t i = 0; i < m_selected.size(); ++i) {
        if (m_selected[i]) result.push_back(static_cast<int>(i));
    }
    return result;
}

std::vector<std::string> GUIMultiDropdown::getSelectedTexts() const {
    std::vector<std::string> result;
    for (size_t i = 0; i < m_selected.size(); ++i) {
        if (m_selected[i]) result.push_back(m_items[i]);
    }
    return result;
}

int GUIMultiDropdown::getSelectedCount() const {
    return static_cast<int>(std::count(m_selected.begin(), m_selected.end(), true));
}

// ============================================================================
// 外观设置
// ============================================================================
void GUIMultiDropdown::setTextColor(RLColor color) { m_textColor = color; }
void GUIMultiDropdown::setNormalColor(RLColor color) { m_normalColor = color; }
void GUIMultiDropdown::setHoverColor(RLColor color) { m_hoverColor = color; }
void GUIMultiDropdown::setExpandedColor(RLColor color) { m_expandedColor = color; }
void GUIMultiDropdown::setItemHoverColor(RLColor color) { m_itemHoverColor = color; }
void GUIMultiDropdown::setFontSize(int fontSize) { m_fontSize = fontSize; }
void GUIMultiDropdown::setRoundness(float roundness) { m_roundness = roundness; }
void GUIMultiDropdown::setMaxVisibleItems(int count) { m_maxVisibleItems = std::max(1, count); }
void GUIMultiDropdown::setOnSelect(SelectCallback callback) { m_onSelect = callback; }

void GUIMultiDropdown::setExpanded(bool expanded) {
    if (m_expanded == expanded) return;
    m_expanded = expanded;
    m_animTarget = expanded ? 1.0f : 0.0f;
}

// ============================================================================
// 碰撞检测（包含展开后的列表区域）
// ============================================================================
bool GUIMultiDropdown::containsPoint(float px, float py) const {
    auto pos = getAbsolutePosition();

    // 主按钮区域
    if (px >= pos.x && px <= pos.x + m_width &&
        py >= pos.y && py <= pos.y + m_height) {
        return true;
    }

    // 展开时的下拉列表区域（使用动画高度）
    if (m_animT > 0.01f) {
        int visibleCount = std::min(m_maxVisibleItems, static_cast<int>(m_items.size()));
        float listHeight = visibleCount * m_height * m_animT;
        if (px >= pos.x && px <= pos.x + m_width &&
            py >= pos.y + m_height && py <= pos.y + m_height + listHeight) {
            return true;
        }
    }

    return false;
}

// ============================================================================
// 获取鼠标悬停的选项索引
// ============================================================================
int GUIMultiDropdown::getHoveredItemIndex() const {
    if (!m_expanded || m_items.empty()) return -1;

    const MouseState& ms = GUIManager::GetInstance().getMouseState();
    const RLVector2& mp = ms.position;
    auto pos = getAbsolutePosition();

    int visibleCount = std::min(m_maxVisibleItems, static_cast<int>(m_items.size()));
    float itemHeight = m_height;

    // 如果滚动条可见，右侧 8px 为滚动条区域，排除
    int totalItems = static_cast<int>(m_items.size());
    float listRight = pos.x + m_width;
    if (totalItems > visibleCount) {
        listRight -= 8;
    }

    for (int i = 0; i < visibleCount; ++i) {
        float iy = pos.y + m_height + i * itemHeight;
        if (mp.x >= pos.x && mp.x <= listRight &&
            mp.y >= iy && mp.y <= iy + itemHeight) {
            return m_scrollOffset + i;
        }
    }
    return -1;
}

// ============================================================================
// 绘制
// ============================================================================
void GUIMultiDropdown::draw() {
    if (!m_visible) return;
    drawButton();

    float animHeightT = m_animT;
    if (animHeightT < 0.01f) return;

    auto pos = getAbsolutePosition();
    int visibleCount = std::min(m_maxVisibleItems, static_cast<int>(m_items.size()));
    float itemHeight = m_height;
    float fullListHeight = visibleCount * itemHeight;
    float currentListHeight = fullListHeight * animHeightT;

    RLBeginScissorMode(
        static_cast<int>(pos.x),
        static_cast<int>(pos.y + m_height),
        static_cast<int>(m_width),
        static_cast<int>(currentListHeight + 2)
    );

    drawDropdownList();

    RLEndScissorMode();
}

void GUIMultiDropdown::drawButton() {
    auto pos = getAbsolutePosition();

    // 选择背景色
    RLColor bgColor = m_normalColor;
    if (!m_enabled) {
        bgColor = RLColor{
            static_cast<unsigned char>(m_normalColor.r / 2),
            static_cast<unsigned char>(m_normalColor.g / 2),
            static_cast<unsigned char>(m_normalColor.b / 2),
            m_normalColor.a
        };
    } else if (m_expanded) {
        bgColor = m_expandedColor;
    } else if (m_mainButtonHovered) {
        bgColor = m_hoverColor;
    }

    // 主按钮背景
    RLRectangle rec{ pos.x, pos.y, m_width, m_height };
    RLDrawRectangleRounded(rec, m_roundness, 8, bgColor);
    RLDrawRectangleRoundedLinesEx(rec, m_roundness, 8, 1.0f,
                                  RLColor{ 100, 100, 150, 255 });

    // 显示选中计数或占位符
    std::string displayText;
    int count = getSelectedCount();
    if (count > 0) {
        displayText = "已选 " + std::to_string(count) + " 项";
    } else {
        displayText = "(多选...)";
    }

    RLVector2 textSize = MeasureTextCustom(displayText.c_str(), m_fontSize, 2);
    float tx = pos.x + 8;
    float ty = pos.y + (m_height - textSize.y) / 2.0f;
    DrawTextCustom(displayText.c_str(), { tx, ty }, m_fontSize, m_textColor, 2);

    // 绘制右侧下拉箭头 (▼)
    float arrowSize = m_height * 0.3f;
    float arrowX = pos.x + m_width - arrowSize - 8;
    float arrowY = pos.y + (m_height - arrowSize) / 2.0f;

    RLVector2 v1{ arrowX, arrowY };
    RLVector2 v2{ arrowX + arrowSize, arrowY };
    RLVector2 v3{ arrowX + arrowSize / 2.0f, arrowY + arrowSize };
    RLDrawTriangle(v1, v2, v3, m_textColor);
}

void GUIMultiDropdown::drawDropdownList() {
    auto pos = getAbsolutePosition();

    int visibleCount = std::min(m_maxVisibleItems, static_cast<int>(m_items.size()));
    float itemHeight = m_height;
    float listWidth = m_width;
    float listHeight = visibleCount * itemHeight;

    // alpha 随动画淡入淡出
    unsigned char listAlpha = static_cast<unsigned char>(235 * m_animT);

    // 下拉列表背景
    RLRectangle listRec{ pos.x, pos.y + m_height, listWidth, listHeight };
    RLDrawRectangleRounded(listRec, m_roundness, 8,
                           RLColor{ 35, 35, 55, listAlpha });
    RLDrawRectangleRoundedLinesEx(listRec, m_roundness, 8, 1.0f,
                                  RLColor{ 100, 100, 150,
                                  static_cast<unsigned char>(255 * m_animT) });

    // 绘制每个选项（考虑滚动偏移）
    int totalItems = static_cast<int>(m_items.size());
    for (int i = 0; i < visibleCount; ++i) {
        int itemIndex = m_scrollOffset + i;
        if (itemIndex >= totalItems) break;

        float iy = pos.y + m_height + i * itemHeight;

        // 悬停高亮
        int hovered = getHoveredItemIndex();
        if (itemIndex == hovered) {
            RLRectangle itemRec{ pos.x + 2, iy + 2, listWidth - 4, itemHeight - 4 };
            RLDrawRectangleRounded(itemRec, m_roundness * 0.5f, 8,
                                   RLColor{ m_itemHoverColor.r, m_itemHoverColor.g,
                                            m_itemHoverColor.b,
                                            static_cast<unsigned char>(m_itemHoverColor.a * m_animT) });
        }

        // 多选复选框指示（☐ / ☑）
        if (m_selected[itemIndex]) {
            DrawTextCustom("☑", { pos.x + 6, iy + (itemHeight - m_fontSize) / 2.0f },
                           m_fontSize, RLColor{ 100, 255, 100,
                           static_cast<unsigned char>(255 * m_animT) }, 2);
        } else {
            DrawTextCustom("☐", { pos.x + 6, iy + (itemHeight - m_fontSize) / 2.0f },
                           m_fontSize, RLColor{ 180, 180, 180,
                           static_cast<unsigned char>(255 * m_animT) }, 2);
        }

        // 选项文本
        RLVector2 textSize = MeasureTextCustom(m_items[itemIndex].c_str(), m_fontSize, 2);
        float tx = pos.x + 28;
        float ty = iy + (itemHeight - textSize.y) / 2.0f;
        DrawTextCustom(m_items[itemIndex].c_str(), { tx, ty }, m_fontSize,
                       RLColor{ m_textColor.r, m_textColor.g, m_textColor.b,
                       static_cast<unsigned char>(255 * m_animT) }, 2);
    }

    // 绘制滚动条（关闭时直接隐藏，不参与淡出动画）
    if (m_expanded && totalItems > visibleCount) {
        m_scrollbar.setRect(listRec.x + listRec.width - 8, listRec.y, 8, listRec.height);
        m_scrollbar.setContent(static_cast<float>(totalItems) * itemHeight, listRec.height);
        m_scrollbar.setThickness(8);
        m_scrollbar.draw();
    }
}

// ============================================================================
// 更新
// ============================================================================
void GUIMultiDropdown::update(float dt) {
    if (!m_visible) {
        m_expanded = false;
        m_animTarget = 0.0f;
        return;
    }

    // 动画插值
    if (std::abs(m_animT - m_animTarget) > 0.005f) {
        m_animT += (m_animTarget - m_animT) * std::min(1.0f, m_animSpeed * dt);
    } else {
        m_animT = m_animTarget;
    }

    const MouseState& ms = GUIManager::GetInstance().getMouseState();
    const RLVector2& mp = ms.position;
    auto pos = getAbsolutePosition();
    bool hitMainButton = (mp.x >= pos.x && mp.x <= pos.x + m_width &&
                          mp.y >= pos.y && mp.y <= pos.y + m_height);

    int totalItems = static_cast<int>(m_items.size());
    int visibleCount = std::min(m_maxVisibleItems, totalItems);
    float itemHeight = m_height;

    // 滚轮 + 滚动条（展开时）
    if (m_expanded) {
        float listHeightF = static_cast<float>(visibleCount) * itemHeight;
        RLRectangle listRec{ pos.x, pos.y + m_height, m_width, listHeightF };
        m_scrollbar.setRect(listRec.x + listRec.width - 8, listRec.y, 8, listRec.height);
        m_scrollbar.setContent(static_cast<float>(totalItems) * itemHeight, listRec.height);
        m_scrollbar.setThickness(8);

        // 先将当前 m_scrollOffset 同步到 ScrollBar，再处理拖拽
        m_scrollbar.setScrollOffset(-static_cast<float>(m_scrollOffset) * itemHeight);
        m_scrollbar.handleScroll(ms);

        float newOffsetPx = m_scrollbar.getScrollOffset();
        int maxScroll = std::max(0, totalItems - visibleCount);
        int newScrollOffset = static_cast<int>(-newOffsetPx / itemHeight + 0.5f);
        newScrollOffset = std::clamp(newScrollOffset, 0, maxScroll);
        m_scrollOffset = newScrollOffset;
    }

    if (ms.leftPressed) {
        if (IsClickConsumed()) return;

        if (hitMainButton && m_enabled) {
            setExpanded(!m_expanded);
            m_scrollOffset = 0;
            ConsumeClick();
        } else if (m_expanded) {
            int hoveredItem = getHoveredItemIndex();
            if (hoveredItem != -1) {
                m_selected[hoveredItem] = !m_selected[hoveredItem];
                if (m_onSelect) {
                    m_onSelect(hoveredItem, m_items[hoveredItem], m_selected[hoveredItem]);
                }
                ConsumeClick();
            } else if (m_scrollbar.isVisible() && mp.x >= pos.x + m_width - 8 && mp.x <= pos.x + m_width &&
                       mp.y >= pos.y + m_height && mp.y <= pos.y + m_height + visibleCount * m_height) {
                ConsumeClick();
            } else {
                setExpanded(false);
            }
        }
    }
}

// ============================================================================
// 鼠标事件回调
// ============================================================================
void GUIMultiDropdown::onMouseEnter() {
    m_mainButtonHovered = true;
}

void GUIMultiDropdown::onMouseLeave() {
    m_mainButtonHovered = false;
}

void GUIMultiDropdown::onMouseWheel(const MouseState& state) {
    if (!m_expanded) return;

    float itemHeight = m_height;
    int totalItems = static_cast<int>(m_items.size());
    int visibleCount = std::min(m_maxVisibleItems, totalItems);
    int maxScroll = std::max(0, totalItems - visibleCount);

    float scrollOffsetPx = -static_cast<float>(m_scrollOffset) * itemHeight;
    scrollOffsetPx += state.wheelMove * itemHeight * 3;

    int newScrollOffset = static_cast<int>(-scrollOffsetPx / itemHeight + 0.5f);
    newScrollOffset = std::clamp(newScrollOffset, 0, maxScroll);
    m_scrollOffset = newScrollOffset;

    // 同步到 ScrollBar，保持滑块位置一致
    m_scrollbar.setScrollOffset(-static_cast<float>(m_scrollOffset) * itemHeight);
}

// ============================================================================
// 焦点
// ============================================================================
void GUIMultiDropdown::onFocus() {
}

void GUIMultiDropdown::onBlur() {
}
