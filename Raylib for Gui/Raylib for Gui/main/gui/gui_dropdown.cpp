#include "gui_dropdown.h"
#include "raylib.h"
#include <algorithm>
#include "../backends/gui/gui_manager.h"
#include "../backends/font/FontManager.h"

// ============================================================================
// GUIDropdown 实现
// ============================================================================
GUIDropdown::GUIDropdown(int id, int zIndex, float x, float y, float width, float height)
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
void GUIDropdown::addItem(const std::string& item) {
    m_items.push_back(item);
}

void GUIDropdown::removeItem(int index) {
    if (index >= 0 && index < static_cast<int>(m_items.size())) {
        m_items.erase(m_items.begin() + index);
        if (m_selectedIndex == index) {
            m_selectedIndex = -1;
        } else if (m_selectedIndex > index) {
            m_selectedIndex--;
        }
    }
}

void GUIDropdown::clearItems() {
    m_items.clear();
    m_selectedIndex = -1;
}

void GUIDropdown::setItems(const std::vector<std::string>& items) {
    m_items = items;
    m_selectedIndex = -1;
}

// ============================================================================
// 选中
// ============================================================================
void GUIDropdown::setSelectedIndex(int index) {
    if (index >= -1 && index < static_cast<int>(m_items.size())) {
        m_selectedIndex = index;
    }
}

std::string GUIDropdown::getSelectedText() const {
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
        return m_items[m_selectedIndex];
    }
    return "";
}

// ============================================================================
// 外观设置
// ============================================================================
void GUIDropdown::setTextColor(RLColor color) { m_textColor = color; }
void GUIDropdown::setNormalColor(RLColor color) { m_normalColor = color; }
void GUIDropdown::setHoverColor(RLColor color) { m_hoverColor = color; }
void GUIDropdown::setExpandedColor(RLColor color) { m_expandedColor = color; }
void GUIDropdown::setItemHoverColor(RLColor color) { m_itemHoverColor = color; }
void GUIDropdown::setFontSize(int fontSize) { m_fontSize = fontSize; }
void GUIDropdown::setRoundness(float roundness) { m_roundness = roundness; }
void GUIDropdown::setMaxVisibleItems(int count) { m_maxVisibleItems = std::max(1, count); }
void GUIDropdown::setOnSelect(SelectCallback callback) { m_onSelect = callback; }

void GUIDropdown::setExpanded(bool expanded) {
    if (m_expanded == expanded) return;
    m_expanded = expanded;
    m_animTarget = expanded ? 1.0f : 0.0f;
}

// ============================================================================
// 碰撞检测（包含展开后的列表区域）
// ============================================================================
bool GUIDropdown::containsPoint(float px, float py) const {
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
int GUIDropdown::getHoveredItemIndex() const {
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
void GUIDropdown::draw() {
    if (!m_visible) return;
    drawButton();

    // 动画完全归零则不绘制列表
    float animHeightT = m_animT; // 用于高度缩放的动画值
    if (animHeightT < 0.01f) return;

    auto pos = getAbsolutePosition();
    int visibleCount = std::min(m_maxVisibleItems, static_cast<int>(m_items.size()));
    float itemHeight = m_height;
    float fullListHeight = visibleCount * itemHeight;
    float currentListHeight = fullListHeight * animHeightT;

    // 用 scissor 裁剪，实现从顶部向下生长/收缩
    RLBeginScissorMode(
        static_cast<int>(pos.x),
        static_cast<int>(pos.y + m_height),
        static_cast<int>(m_width),
        static_cast<int>(currentListHeight + 2) // +2 防止圆角边缘裁切
    );

    // 在完整高度位置绘制，scissor 自动裁剪
    drawDropdownList();

    RLEndScissorMode();
}

void GUIDropdown::drawButton() {
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

    // 显示选中的文本或占位符
    std::string displayText = getSelectedText();
    if (displayText.empty()) {
        displayText = "(请选择...)";
    }

    RLVector2 textSize = MeasureTextCustom(displayText.c_str(), m_fontSize, 2);
    float tx = pos.x + 8;
    float ty = pos.y + (m_height - textSize.y) / 2.0f;
    DrawTextCustom(displayText.c_str(), { tx, ty }, m_fontSize, m_textColor, 2);

    // 绘制右侧下拉箭头 (▼)
    float arrowSize = m_height * 0.3f;
    float arrowX = pos.x + m_width - arrowSize - 8;
    float arrowY = pos.y + (m_height - arrowSize) / 2.0f;

    // 用三角形绘制箭头
    RLVector2 v1{ arrowX, arrowY };
    RLVector2 v2{ arrowX + arrowSize, arrowY };
    RLVector2 v3{ arrowX + arrowSize / 2.0f, arrowY + arrowSize };
    RLDrawTriangle(v1, v2, v3, m_textColor);
}

void GUIDropdown::drawDropdownList() {
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

        // 选中标记
        if (itemIndex == m_selectedIndex) {
            DrawTextCustom("✓", { pos.x + 6, iy + (itemHeight - m_fontSize) / 2.0f },
                           m_fontSize, RLColor{ 100, 255, 100,
                           static_cast<unsigned char>(255 * m_animT) }, 2);
        }

        // 选项文本
        RLVector2 textSize = MeasureTextCustom(m_items[itemIndex].c_str(), m_fontSize, 2);
        float tx = pos.x + 24;
        float ty = iy + (itemHeight - textSize.y) / 2.0f;
        DrawTextCustom(m_items[itemIndex].c_str(), { tx, ty }, m_fontSize,
                       RLColor{ m_textColor.r, m_textColor.g, m_textColor.b,
                       static_cast<unsigned char>(255 * m_animT) }, 2);
    }

    // 绘制滚动条（关闭时直接隐藏，不参与淡出动画）
    if (m_expanded && totalItems > visibleCount) {
        // 滚动条区域位置 = 下拉列表右侧
        m_scrollbar.setRect(listRec.x + listRec.width - 8, listRec.y, 8, listRec.height);
        m_scrollbar.setContent(static_cast<float>(totalItems) * itemHeight, listRec.height);
        m_scrollbar.setThickness(8);
        m_scrollbar.draw();
    }
}

// ============================================================================
// 更新
// ============================================================================
void GUIDropdown::update(float dt) {
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
    // 单独检测主按钮区域（不包含下拉列表，避免与 containsPoint 重写冲突）
    bool hitMainButton = (mp.x >= pos.x && mp.x <= pos.x + m_width &&
                          mp.y >= pos.y && mp.y <= pos.y + m_height);

    int totalItems = static_cast<int>(m_items.size());
    int visibleCount = std::min(m_maxVisibleItems, totalItems);
    float itemHeight = m_height;

    // 滚轮 + 滚动条拖拽（展开时）
    if (m_expanded) {
        // 滚动处理已移到 onMouseWheel，此处仅更新滚动条状态
        float listHeightF = static_cast<float>(visibleCount) * itemHeight;
        RLRectangle listRec{ pos.x, pos.y + m_height, m_width, listHeightF };
        m_scrollbar.setRect(listRec.x + listRec.width - 8, listRec.y, 8, listRec.height);
        m_scrollbar.setContent(static_cast<float>(totalItems) * itemHeight, listRec.height);
        m_scrollbar.setThickness(8);

        // 先将当前 m_scrollOffset 同步到 ScrollBar，再处理拖拽
        m_scrollbar.setScrollOffset(-static_cast<float>(m_scrollOffset) * itemHeight);
        // 让 ScrollBar 处理鼠标交互（拖拽），wheel 由 onMouseWheel 处理
        m_scrollbar.handleScroll(ms);

        // 从 ScrollBar 同步回行索引偏移
        float newOffsetPx = m_scrollbar.getScrollOffset();
        int maxScroll = std::max(0, totalItems - visibleCount);
        int newScrollOffset = static_cast<int>(-newOffsetPx / itemHeight + 0.5f);
        newScrollOffset = std::clamp(newScrollOffset, 0, maxScroll);
        m_scrollOffset = newScrollOffset;
    }

    // 点击处理
    if (ms.leftPressed) {
        // 如果本帧已有下拉框消耗了点击，跳过防止穿透
        if (IsClickConsumed()) return;

        if (hitMainButton && m_enabled) {
            // 点击主按钮 → 切换展开/收起，重置滚动
            setExpanded(!m_expanded);
            m_scrollOffset = 0;
            ConsumeClick();
        } else if (m_expanded) {
            // 展开状态下点击下拉列表中的选项
            int hoveredItem = getHoveredItemIndex();
            if (hoveredItem != -1) {
                m_selectedIndex = hoveredItem;
                setExpanded(false);
                if (m_onSelect) {
                    m_onSelect(m_selectedIndex, m_items[m_selectedIndex]);
                }
                ConsumeClick();
            } else if (m_scrollbar.isVisible() && mp.x >= pos.x + m_width - 8 && mp.x <= pos.x + m_width &&
                       mp.y >= pos.y + m_height && mp.y <= pos.y + m_height + visibleCount * m_height) {
                // 点击滚动条区域，不关闭下拉框
                ConsumeClick();
            } else {
                // 点击其他地方收起
                setExpanded(false);
            }
        }
    }
}

// ============================================================================
// 鼠标事件回调
// ============================================================================
void GUIDropdown::onMouseEnter() {
    m_mainButtonHovered = true;
}

void GUIDropdown::onMouseLeave() {
    m_mainButtonHovered = false;
}

void GUIDropdown::onMouseWheel(const MouseState& state) {
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

    // 同步到 ScrollBar，保持滑块位置一致（next update() 时会再 setContent 刷新）
    m_scrollbar.setScrollOffset(-static_cast<float>(m_scrollOffset) * itemHeight);
}

// ============================================================================
// 焦点
// ============================================================================
void GUIDropdown::onFocus() {
    // 聚焦时无特殊行为
}

void GUIDropdown::onBlur() {
    // 失焦时不收起下拉列表，由 update() 中的点击检测来管理收起
}
