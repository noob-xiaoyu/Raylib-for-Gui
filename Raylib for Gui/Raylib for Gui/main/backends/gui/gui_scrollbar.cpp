#include "gui_scrollbar.h"
#include "gui_widget.h"
#include <algorithm>

inline float clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

ScrollBar::ScrollBar()
    : m_x(0), m_y(0), m_width(12), m_height(100)
    , m_contentSize(0), m_viewportSize(100)
    , m_scrollOffset(0), m_maxScroll(0)
    , m_thickness(12), m_minThumbSize(20)
    , m_thumbY(0), m_thumbHeight(20), m_trackHeight(0)
    , m_visible(false), m_isDragging(false), m_dragOffset(0)
    , m_orientation(ScrollBarOrientation::Vertical)
    , m_trackColor{ 45, 45, 45, 255 }
    , m_thumbColor{ 160, 160, 160, 255 }
    , m_thumbHoverColor{ 180, 180, 180, 255 }
    , m_thumbDragColor{ 200, 200, 200, 255 }
{
}

void ScrollBar::setRect(float x, float y, float width, float height) {
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

void ScrollBar::setContent(float contentSize, float viewportSize) {
    m_contentSize = contentSize;
    m_viewportSize = viewportSize;

    m_visible = m_contentSize > m_viewportSize;
    m_maxScroll = m_visible ? (m_contentSize - m_viewportSize) : 0;

    if (m_scrollOffset > 0) m_scrollOffset = 0;
    if (m_scrollOffset < -m_maxScroll) m_scrollOffset = -m_maxScroll;

    updateThumb();
}

void ScrollBar::handleScroll(const MouseState& ms) {
    float mx = ms.position.x;
    float my = ms.position.y;

    bool isHorizontal = (m_orientation == ScrollBarOrientation::Horizontal);

    // 滑块拖拽启动
    if (ms.leftPressed) {
        RLRectangle thumbRect;
        if (isHorizontal) {
            thumbRect = { m_thumbY, m_y + 1, m_thumbHeight, m_height - 2 };
        } else {
            thumbRect = { m_x + 1, m_thumbY, m_width - 2, m_thumbHeight };
        }
        if (RLCheckCollisionPointRec({ mx, my }, thumbRect)) {
            m_isDragging = true;
            if (isHorizontal) {
                m_dragOffset = mx - m_thumbY;
            } else {
                m_dragOffset = my - m_thumbY;
            }
        }
    }

    // 拖拽中
    if (m_isDragging) {
        if (ms.leftReleased) {
            m_isDragging = false;
        } else {
            if (isHorizontal) {
                float trackWidth = m_width;
                float mouseXRelative = mx - m_x - m_dragOffset;
                float newPercent = mouseXRelative / (trackWidth - m_thumbHeight);
                newPercent = clampf(newPercent, 0.0f, 1.0f);
                m_scrollOffset = -newPercent * m_maxScroll;
            } else {
                float trackHeight = m_height;
                float mouseYRelative = my - m_y - m_dragOffset;
                float newPercent = mouseYRelative / (trackHeight - m_thumbHeight);
                newPercent = clampf(newPercent, 0.0f, 1.0f);
                m_scrollOffset = -newPercent * m_maxScroll;
            }
            updateThumb();
        }
    }
}

void ScrollBar::draw() const {
    if (!m_visible) return;

    bool isHorizontal = (m_orientation == ScrollBarOrientation::Horizontal);

    if (isHorizontal) {
        // 水平轨道
        RLRectangle trackRec{ m_x, m_y, m_width, m_height };
        RLDrawRectangleRec(trackRec, m_trackColor);
        // 水平滑块
        RLColor thumbColor = m_isDragging ? m_thumbDragColor : m_thumbColor;
        RLRectangle thumbRec{ m_thumbY, m_y, m_thumbHeight, m_height };
        RLDrawRectangleRounded(thumbRec, 0.5f, 4, thumbColor);
    } else {
        // 垂直轨道
        RLRectangle trackRec{ m_x, m_y, m_width, m_height };
        RLDrawRectangleRec(trackRec, m_trackColor);
        // 垂直滑块
        RLColor thumbColor = m_isDragging ? m_thumbDragColor : m_thumbColor;
        RLRectangle thumbRec{ m_x, m_thumbY, m_width, m_thumbHeight };
        RLDrawRectangleRounded(thumbRec, 0.5f, 4, thumbColor);
    }
}

float ScrollBar::getProgress() const {
    if (m_maxScroll <= 0) return 0;
    return -m_scrollOffset / m_maxScroll;
}

void ScrollBar::updateThumb() {
    if (!m_visible) return;

    bool isHorizontal = (m_orientation == ScrollBarOrientation::Horizontal);

    float viewSize = isHorizontal ? m_width : m_height;
    float ratio = viewSize / m_contentSize;
    m_thumbHeight = viewSize * ratio;
    if (m_thumbHeight < m_minThumbSize) m_thumbHeight = m_minThumbSize;

    m_trackHeight = viewSize;

    float scrollPercent = (m_maxScroll > 0) ? -m_scrollOffset / m_maxScroll : 0.0f;

    if (isHorizontal) {
        m_thumbY = m_x + (m_trackHeight - m_thumbHeight) * scrollPercent;
    } else {
        m_thumbY = m_y + (m_trackHeight - m_thumbHeight) * scrollPercent;
    }
}
