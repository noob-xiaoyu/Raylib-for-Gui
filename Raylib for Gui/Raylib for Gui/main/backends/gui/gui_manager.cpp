#include "gui_manager.h"
#include "../../gui/gui_dropdown.h"
#include "../../gui/gui_multidropdown.h"
#include "../../gui/gui_TextBox.h"
#include "../logsys/logs.h"
#include "../font/FontManager.h"
#include <algorithm>
#include <cmath>

// ============================================================================
// 单例
// ============================================================================
GUIManager& GUIManager::GetInstance() {
    static GUIManager instance;
    return instance;
}

// ============================================================================
// 控件管理
// ============================================================================
void GUIManager::removeWidget(int id) {
    m_widgets.erase(id);
    m_zIndex.removeElement(id);

    auto fit = std::find(m_focusableOrder.begin(), m_focusableOrder.end(), id);
    if (fit != m_focusableOrder.end()) {
        m_focusableOrder.erase(fit);
    }

    if (m_focusedId == id) m_focusedId = -1;
    if (m_hoveredId == id) m_hoveredId = -1;
    if (m_prevHoveredId == id) m_prevHoveredId = -1;
    if (m_pressedId == id) m_pressedId = -1;
}

void GUIManager::addWidget(GUIWidget* widget) {
    if (!widget) return;
    int id = widget->getId();

    if (m_widgets.find(id) != m_widgets.end()) return;

    m_widgets[id] = std::unique_ptr<GUIWidget>(widget);
    m_zIndex.addElement(id, widget->getZIndex());

    if (widget->canBeFocused()) {
        m_focusableOrder.push_back(id);
    }
}

GUIWidget* GUIManager::getWidget(int id) const {
    auto it = m_widgets.find(id);
    return (it != m_widgets.end()) ? it->second.get() : nullptr;
}

// ============================================================================
// 事件处理
// ============================================================================
void GUIManager::handleEvents() {
    // 1. 采样鼠标状态快照（所有后续鼠标事件基于此快照，保证帧内一致性）
    RLVector2 wheelV = RLGetMouseWheelMoveV();
    m_mouseState = {
        RLGetMousePosition(),
        RLGetMouseDelta(),
        wheelV.y,               // wheelMove: 垂直滚轮
        wheelV.x,               // wheelMoveH: 水平滚轮
        RLIsMouseButtonPressed(MOUSE_LEFT_BUTTON),
        RLIsMouseButtonReleased(MOUSE_LEFT_BUTTON),
        RLIsMouseButtonDown(MOUSE_LEFT_BUTTON),
        RLIsMouseButtonPressed(MOUSE_RIGHT_BUTTON),
        RLIsMouseButtonReleased(MOUSE_RIGHT_BUTTON),
        RLIsMouseButtonDown(MOUSE_RIGHT_BUTTON)
    };

    handleMouseInput();
    handleKeyboardInput();
}

void GUIManager::handleMouseInput() {
    m_clickConsumed = false;

    const auto& renderOrder = m_zIndex.getRenderOrder();
    const RLVector2& mp = m_mouseState.position;

    // ---- 命中检测（从上往下找第一个命中、可见、可用的控件） ----
    int newHovered = -1;
    for (int i = static_cast<int>(renderOrder.size()) - 1; i >= 0; --i) {
        int id = renderOrder[i];
        auto it = m_widgets.find(id);
        if (it == m_widgets.end() || !it->second->isVisible() || !it->second->isEnabled())
            continue;
        if (it->second->containsPoint(mp.x, mp.y)) {
            newHovered = id;
            break;
        }
    }
    m_hoveredId = newHovered;

    // ---- Hover 过渡事件派发（enter / leave） ----
    if (m_prevHoveredId != m_hoveredId) {
        // 离开旧控件
        if (m_prevHoveredId != -1) {
            auto it = m_widgets.find(m_prevHoveredId);
            if (it != m_widgets.end()) {
                it->second->onMouseLeave();
            }
        }
        // 进入新控件
        if (m_hoveredId != -1) {
            auto it = m_widgets.find(m_hoveredId);
            if (it != m_widgets.end()) {
                it->second->onMouseEnter();
            }
        }
        m_prevHoveredId = m_hoveredId;
    }

    // ---- 鼠标移动事件 ----
    if (m_hoveredId != -1) {
        auto it = m_widgets.find(m_hoveredId);
        if (it != m_widgets.end()) {
            it->second->onMouseMove(m_mouseState);
        }
    }
    // 拖拽中持续向按下的控件发送鼠标移动（支持滑块等拖拽离开区域）
    if (m_pressedId != -1 && m_pressedId != m_hoveredId) {
        auto it = m_widgets.find(m_pressedId);
        if (it != m_widgets.end()) {
            it->second->onMouseMove(m_mouseState);
        }
    }

    // ---- 按下 ----
    if (m_mouseState.leftPressed) {
        if (m_hoveredId != -1) {
            m_pressedId = m_hoveredId;
            auto it = m_widgets.find(m_hoveredId);
            if (it != m_widgets.end()) {
                it->second->onMouseDown(m_mouseState);
            }
        }
    }

    // ---- 释放 + 单击检测 ----
    if (m_mouseState.leftReleased) {
        if (m_pressedId != -1) {
            auto it = m_widgets.find(m_pressedId);
            if (it != m_widgets.end()) {
                // 始终发送 onMouseUp
                it->second->onMouseUp(m_mouseState);
                // 抬起时若鼠标仍在同一控件上 → 发送 onClick
                if (m_pressedId == m_hoveredId) {
                    it->second->onClick(m_mouseState);
                }
            }
        }
        m_pressedId = -1;
    }

    // ---- 滚轮事件 ----
    if (m_mouseState.wheelMove != 0.0f && m_hoveredId != -1) {
        auto it = m_widgets.find(m_hoveredId);
        if (it != m_widgets.end()) {
            it->second->onMouseWheel(m_mouseState);
        }
    }

    // ---- 全局点击对焦（点击可聚焦控件时设置焦点，点击空白区域清除焦点） ----
    if (m_mouseState.leftPressed) {
        if (m_hoveredId != -1) {
            bool skipClickFocus = false;

            // TextBox 自行管理焦点
            auto hoveredIt = m_widgets.find(m_hoveredId);
            if (hoveredIt != m_widgets.end()) {
                TextBox* tb = dynamic_cast<TextBox*>(hoveredIt->second.get());
                if (tb) skipClickFocus = true;
            }

            // 点击展开下拉框的扩展列表时不对焦
            if (!skipClickFocus && m_expandedDropdownId != -1 && m_hoveredId == m_expandedDropdownId) {
                auto ddIt = m_widgets.find(m_expandedDropdownId);
                if (ddIt != m_widgets.end()) {
                    auto pos = ddIt->second->getAbsolutePosition();
                    if (mp.y > pos.y + ddIt->second->getHeight())
                        skipClickFocus = true;
                }
            }

            if (!skipClickFocus) {
                setFocus(m_hoveredId);
            }
        } else {
            clearFocus();
        }
    }
}

// ============================================================================
// 键盘输入处理（焦点导航）
// ============================================================================
void GUIManager::handleKeyboardInput() {
    if (m_focusableOrder.empty()) return;

    if (RLIsKeyPressed(RL_E_KEY_TAB)) {
        if (RLIsKeyDown(RL_E_KEY_LEFT_SHIFT) || RLIsKeyDown(RL_E_KEY_RIGHT_SHIFT)) {
            focusPrevious();
        } else {
            focusNext();
        }
    }

    // Enter 激活焦点控件的 onClick（通用，不再依赖 GUIButton 特有方法）
    if (m_focusedId != -1 && RLIsKeyPressed(RL_E_KEY_ENTER)) {
        auto it = m_widgets.find(m_focusedId);
        if (it != m_widgets.end()) {
            it->second->onClick(m_mouseState);
        }
    }
}

// ============================================================================
// 焦点管理
// ============================================================================
void GUIManager::setFocus(int id) {
    if (m_focusedId == id) return;

    if (m_focusedId != -1) {
        auto it = m_widgets.find(m_focusedId);
        if (it != m_widgets.end()) {
            it->second->setFocused(false);
        }
    }

    m_focusedId = id;

    if (id != -1) {
        auto it = m_widgets.find(id);
        if (it != m_widgets.end() && it->second->canBeFocused()) {
            it->second->setFocused(true);

            // 捕捉新焦点框作为动画目标（带 padding）
            RLRectangle newRect = it->second->getFocusRect();
            float p = 3.0f;
            newRect.x -= p; newRect.y -= p;
            newRect.width += p * 2; newRect.height += p * 2;
            m_focusTargetRect = newRect;

            // 如果 anim 矩形还是零（首次聚焦），直接跳到目标
            if (m_focusAnimRect.width < 1.0f && m_focusAnimRect.height < 1.0f) {
                m_focusAnimRect = m_focusTargetRect;
            }
        } else {
            m_focusedId = -1;
        }
    }
    // 失焦：不改变 m_focusTargetRect，让 alpha 淡出
}

void GUIManager::clearFocus() {
    if (m_focusedId != -1) {
        setFocus(-1); // 失焦后矩形位置保留，alpha 淡出
    }
}

GUIWidget* GUIManager::getFocusedWidget() const {
    if (m_focusedId == -1) return nullptr;
    auto it = m_widgets.find(m_focusedId);
    return (it != m_widgets.end()) ? it->second.get() : nullptr;
}

void GUIManager::focusNext() {
    if (m_focusableOrder.empty()) return;

    int currentFocus = m_focusedId;
    auto it = std::find(m_focusableOrder.begin(), m_focusableOrder.end(), currentFocus);

    if (it == m_focusableOrder.end() || ++it == m_focusableOrder.end()) {
        setFocus(m_focusableOrder.front());
    } else {
        setFocus(*it);
    }
}

void GUIManager::focusPrevious() {
    if (m_focusableOrder.empty()) return;

    int currentFocus = m_focusedId;
    auto it = std::find(m_focusableOrder.begin(), m_focusableOrder.end(), currentFocus);

    if (it == m_focusableOrder.begin() || it == m_focusableOrder.end()) {
        setFocus(m_focusableOrder.back());
    } else {
        setFocus(*--it);
    }
}

// ============================================================================
// 展开下拉框检测
// ============================================================================
bool GUIManager::isPointOnExpandedDropdown(float px, float py) const {
    if (m_expandedDropdownId == -1) return false;
    auto it = m_widgets.find(m_expandedDropdownId);
    if (it == m_widgets.end() || !it->second->isVisible()) return false;
    return it->second->containsPoint(px, py);
}

// ============================================================================
// 焦点指示器绘制
// ============================================================================
void GUIManager::drawWidgetFocus(int id) {
    if (id != m_focusedId) return;

    auto it = m_widgets.find(id);
    if (it == m_widgets.end() || !it->second->canBeFocused()) return;

    // 使用平滑动画矩形（阶段2已包含下拉框列表扩展）
    RLRectangle focusRect = m_focusAnimRect;

    float pulse = sinf((float)RLGetTime() * 4.0f) * 0.3f + 0.7f;
    unsigned char alpha = static_cast<unsigned char>(180 * pulse * m_focusAlpha);
    RLColor focusColor = RLColor{ 255, 255, 100, alpha };

    RLDrawRectangleRoundedLinesEx(focusRect, 0.03f, 8, 2.0f, focusColor);
}

// ============================================================================
// Z-Index 同步工具
// ============================================================================
void GUIManager::syncWidgetZIndex(int id) {
    auto it = m_widgets.find(id);
    if (it != m_widgets.end()) {
        it->second->setZIndex(m_zIndex.getZIndex(id));
    }
}

// ============================================================================
// 渲染
// ============================================================================
void GUIManager::render() {
    float dt = RLGetFrameTime();
    const auto& renderOrder = m_zIndex.getRenderOrder();

    // ---- 阶段1：更新（从顶层到底层反向遍历） ----
    for (int i = static_cast<int>(renderOrder.size()) - 1; i >= 0; --i) {
        int id = renderOrder[i];
        auto it = m_widgets.find(id);
        if (it != m_widgets.end() && it->second->isVisible()) {
            it->second->update(dt);
        }
    }

    // ---- 阶段2：焦点框平滑动画（含下拉框列表扩展） ----
    {
        RLRectangle& a = m_focusAnimRect;
        // 计算实际目标矩形：如果有焦点的控件是下拉框且正在展开，扩展高度包含列表
        RLRectangle effectiveTarget = m_focusTargetRect;
        if (m_focusedId != -1) {
            auto fit = m_widgets.find(m_focusedId);
            if (fit != m_widgets.end()) {
                GUIDropdown* dd = dynamic_cast<GUIDropdown*>(fit->second.get());
                GUIMultiDropdown* mdd = dynamic_cast<GUIMultiDropdown*>(fit->second.get());
                float animT = dd ? dd->getAnimT() : (mdd ? mdd->getAnimT() : 0.0f);
                if (animT > 0.01f) {
                    int itemCount = dd ? (int)dd->getItemCount() : (int)mdd->getItemCount();
                    int maxVisible = dd ? dd->getMaxVisibleItems() : mdd->getMaxVisibleItems();
                    int visibleCount = std::min(maxVisible, itemCount);
                    float listHeight = visibleCount * fit->second->getHeight() * animT;
                    effectiveTarget.height += listHeight;
                }
            }
        }
        const RLRectangle& t = effectiveTarget;
        // 位置/大小插值
        if (std::abs(a.x - t.x) > 0.5f || std::abs(a.y - t.y) > 0.5f ||
            std::abs(a.width - t.width) > 0.5f || std::abs(a.height - t.height) > 0.5f) {
            float speed = std::min(1.0f, m_focusAnimSpeed * dt);
            a.x += (t.x - a.x) * speed;
            a.y += (t.y - a.y) * speed;
            a.width += (t.width - a.width) * speed;
            a.height += (t.height - a.height) * speed;
        } else {
            a = t;
        }
        // alpha 插值：有焦点 → 1.0，无焦点 → 0.0
        float targetAlpha = (m_focusedId != -1) ? 1.0f : 0.0f;
        if (std::abs(m_focusAlpha - targetAlpha) > 0.005f) {
            m_focusAlpha += (targetAlpha - m_focusAlpha) * std::min(1.0f, m_focusAnimSpeed * dt);
        } else {
            m_focusAlpha = targetAlpha;
        }
    }

    // ---- 阶段3：下拉框展开/收起自动置顶/恢复 ----
    for (size_t i = 0; i < renderOrder.size(); ++i) {
        int id = renderOrder[i];
        auto it = m_widgets.find(id);
        if (it == m_widgets.end() || !it->second->isVisible()) continue;

        GUIDropdown* dd = dynamic_cast<GUIDropdown*>(it->second.get());
        GUIMultiDropdown* mdd = dynamic_cast<GUIMultiDropdown*>(it->second.get());
        if (!dd && !mdd) continue;

        bool expanded = dd ? dd->isExpanded() : mdd->isExpanded();

        if (expanded && m_expandedDropdownId != id) {
            // 新下拉框展开 → 收起旧的，置顶新的
            if (m_expandedDropdownId != -1) {
                auto oldIt = m_widgets.find(m_expandedDropdownId);
                if (oldIt != m_widgets.end()) {
                    GUIDropdown* oldDd = dynamic_cast<GUIDropdown*>(oldIt->second.get());
                    if (oldDd) oldDd->setExpanded(false);
                    GUIMultiDropdown* oldMdd = dynamic_cast<GUIMultiDropdown*>(oldIt->second.get());
                    if (oldMdd) oldMdd->setExpanded(false);
                }
                setZIndex(m_expandedDropdownId, m_savedDropdownZIndex);
            }
            m_savedDropdownZIndex = it->second->getZIndex();
            m_expandedDropdownId = id;
            bringToFront(id);
        } else if (!expanded && m_expandedDropdownId == id) {
            // 下拉框收起 → 恢复原始 z-index
            setZIndex(id, m_savedDropdownZIndex);
            m_expandedDropdownId = -1;
        }
    }

    // ---- 阶段3：绘制（ZIndexManager 已按 z-index 升序排列，从底层到顶层） ----
    for (int id : renderOrder) {
        auto it = m_widgets.find(id);
        if (it != m_widgets.end() && it->second->isVisible()) {
            it->second->draw();
        }
    }

    // ---- 阶段4：焦点指示器（使用平滑动画矩形） ----
    if (m_focusedId != -1) {
        drawWidgetFocus(m_focusedId);
    } else if (m_focusAlpha > 0.01f) {
        // 失焦后 alpha 淡出期间继续绘制
        float pulse = sinf((float)RLGetTime() * 4.0f) * 0.3f + 0.7f;
        unsigned char alpha = static_cast<unsigned char>(180 * pulse * m_focusAlpha);
        RLColor focusColor = RLColor{ 255, 255, 100, alpha };
        RLDrawRectangleRoundedLinesEx(m_focusAnimRect, 0.03f, 8, 2.0f, focusColor);
    }
}

void GUIManager::renderDebug() {
    std::string debug = "Hovered ID: " + std::to_string(m_hoveredId);
    debug += " | Widgets: " + std::to_string(m_widgets.size());
    DrawTextCustom(debug.c_str(), { 10, 10 }, 16, GREEN, 0);
}

// ============================================================================
// Z-Index 操作
// ============================================================================
void GUIManager::bringToFront(int id) {
    m_zIndex.bringToFront(id);
    syncWidgetZIndex(id);
}

void GUIManager::sendToBack(int id) {
    m_zIndex.sendToBack(id);
    syncWidgetZIndex(id);
}

void GUIManager::moveUp(int id) {
    m_zIndex.moveUp(id);
    syncWidgetZIndex(id);
}

void GUIManager::moveDown(int id) {
    m_zIndex.moveDown(id);
    syncWidgetZIndex(id);
}

void GUIManager::setZIndex(int id, int newZ) {
    m_zIndex.setZIndex(id, newZ);
    syncWidgetZIndex(id);
}

// ============================================================================
// 热更新布局
// ============================================================================
void GUIManager::applyAllLayout(float parentW, float parentH) {
    for (auto& [id, widget] : m_widgets) {
        if (widget && widget->getLayoutData().anchors != Layout_None) {
            widget->applyLayout(parentW, parentH);
        }
    }
}

// ============================================================================
// 布局工具
// ============================================================================
void GUIManager::centerWidget(int id, float parentW, float parentH) {
    auto it = m_widgets.find(id);
    if (it == m_widgets.end()) return;

    float x = (parentW - it->second->getWidth()) / 2.0f;
    float y = (parentH - it->second->getHeight()) / 2.0f;
    it->second->setPosition(x, y);
}
