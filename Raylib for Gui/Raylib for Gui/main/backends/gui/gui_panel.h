#ifndef GUI_PANEL_H
#define GUI_PANEL_H

#include "gui_widget.h"
#include "../effects/VisualEffects.h"

// ============================================================================
// GUIPanel - 容器面板（支持软阴影）
// ============================================================================
class GUIPanel : public GUIWidget {
public:
    GUIPanel(int id, int zIndex, float x, float y, float width, float height);

    void draw() override;

    void setBorderColor(RLColor color);
    void setBorderWidth(float width);
    void setRoundness(float roundness);

    // 阴影控制
    void setShadowStrength(float strength);  // 0.0 ~ 1.0（默认 0.0 = 无阴影）
    float getShadowStrength() const { return m_shadowStrength; }

    void setShadowColor(RLColor color);
    void setShadowRadius(float radius);

private:
    RLColor m_borderColor;
    float m_borderWidth;
    float m_roundness;

    // 阴影参数
    float   m_shadowStrength;
    RLColor m_shadowColor;
    float   m_shadowRadius;
};

#endif // GUI_PANEL_H
