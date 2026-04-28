#include "gui_widget.h"

// ============================================================================
// GUIWidget 基类实现
// ============================================================================
GUIWidget::GUIWidget(int id, int zIndex)
    : m_id(id)
    , m_zIndex(zIndex)
    , m_x(0), m_y(0)
    , m_width(100), m_height(50)
    , m_visible(true)
    , m_enabled(true)
    , m_color(WHITE)
    , m_tag("")
    , m_parent(nullptr)
{
}

bool GUIWidget::containsPoint(float px, float py) const {
    auto absPos = getAbsolutePosition();
    return px >= absPos.x && px <= absPos.x + m_width &&
           py >= absPos.y && py <= absPos.y + m_height;
}

RLVector2 GUIWidget::getAbsolutePosition() const {
    if (m_parent) {
        auto parentPos = m_parent->getAbsolutePosition();
        return { parentPos.x + m_x, parentPos.y + m_y };
    }
    return { m_x, m_y };
}

void GUIWidget::setPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

void GUIWidget::setSize(float w, float h) {
    m_width = w;
    m_height = h;
}

// ============================================================================
// 布局锚点系统实现
// ============================================================================
void GUIWidget::setAnchors(int anchorFlags, float parentW, float parentH) {
    m_layout.anchors = anchorFlags;
    m_layout.initX   = m_x;
    m_layout.initY   = m_y;
    m_layout.initW   = m_width;
    m_layout.initH   = m_height;
    m_layout.parentW = parentW;
    m_layout.parentH = parentH;

    // 计算各边距
    if (anchorFlags & Layout_Left)   m_layout.marginL = m_x;
    if (anchorFlags & Layout_Top)    m_layout.marginT = m_y;
    if (anchorFlags & Layout_Right)  m_layout.marginR = parentW - (m_x + m_width);
    if (anchorFlags & Layout_Bottom) m_layout.marginB = parentH - (m_y + m_height);
}

void GUIWidget::applyLayout(float parentW, float parentH) {
    int a = m_layout.anchors;
    // 仅在显式设置了锚点且父容器尺寸大于 0 时生效
    if (a == Layout_None) return;
    if (parentW <= 0 || parentH <= 0) return;

    float newX = m_x, newY = m_y;
    float newW = m_width, newH = m_height;

    // --- 水平方向 ---
    bool hasLeft  = (a & Layout_Left)  != 0;
    bool hasRight = (a & Layout_Right) != 0;

    if (hasLeft && hasRight) {
        // 拉伸模式：左边缘距左固定，右边缘距右固定
        float scaleX = (m_layout.parentW > 0) ? parentW / m_layout.parentW : 1.0f;
        newX = m_layout.marginL * scaleX;
        newW = (parentW - m_layout.marginR * scaleX) - newX;
        // 检查翻转保护
        if (newW < 4.0f) newW = 4.0f;
        if (newX + newW > parentW) newX = parentW - newW;
    } else if (hasLeft && !hasRight) {
        // 仅左锚定：按父容器宽度比例缩放
        if (m_layout.parentW > 0) {
            float scaleX = parentW / m_layout.parentW;
            newX = m_layout.marginL * scaleX;
            if (!m_keepSize) newW = m_layout.initW * scaleX;
        }
    } else if (!hasLeft && hasRight) {
        // 仅右锚定：距右边缘固定
        if (m_layout.parentW > 0) {
            float scaleX = parentW / m_layout.parentW;
            if (!m_keepSize) newW = m_layout.initW * scaleX;
            newX = parentW - m_layout.marginR * scaleX - newW;
        }
    }

    // --- 垂直方向 ---
    bool hasTop    = (a & Layout_Top)    != 0;
    bool hasBottom = (a & Layout_Bottom) != 0;

    if (hasTop && hasBottom) {
        // 拉伸模式
        float scaleY = (m_layout.parentH > 0) ? parentH / m_layout.parentH : 1.0f;
        newY = m_layout.marginT * scaleY;
        newH = (parentH - m_layout.marginB * scaleY) - newY;
        if (newH < 4.0f) newH = 4.0f;
        if (newY + newH > parentH) newY = parentH - newH;
    } else if (hasTop && !hasBottom) {
        // 仅顶锚定
        if (m_layout.parentH > 0) {
            float scaleY = parentH / m_layout.parentH;
            newY = m_layout.marginT * scaleY;
            if (!m_keepSize) newH = m_layout.initH * scaleY;
        }
    } else if (!hasTop && hasBottom) {
        // 仅底锚定
        if (m_layout.parentH > 0) {
            float scaleY = parentH / m_layout.parentH;
            if (!m_keepSize) newH = m_layout.initH * scaleY;
            newY = parentH - m_layout.marginB * scaleY - newH;
        }
    }

    // 应用
    setPosition(newX, newY);
    setSize(newW, newH);
}

void GUIWidget::setVisible(bool visible) {
    m_visible = visible;
}

void GUIWidget::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void GUIWidget::setZIndex(int zIndex) {
    m_zIndex = zIndex;
}

void GUIWidget::setParent(GUIWidget* parent) {
    m_parent = parent;
}

void GUIWidget::setColor(RLColor color) {
    m_color = color;
}

void GUIWidget::setTag(const std::string& tag) {
    m_tag = tag;
}

void GUIWidget::setFocused(bool focused) {
    if (m_focused == focused) return;
    m_focused = focused;
    if (focused) onFocus();
    else onBlur();
}

RLRectangle GUIWidget::getBounds() const {
    auto pos = getAbsolutePosition();
    return { pos.x, pos.y, m_width, m_height };
}

RLRectangle GUIWidget::getFocusRect() const {
    return getBounds();
}
