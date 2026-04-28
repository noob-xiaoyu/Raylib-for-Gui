#ifndef GUI_LABEL_H
#define GUI_LABEL_H

#include "../backends/gui/gui_widget.h"
#include <string>

// ============================================================================
// GUILabel - 文本标签
// ============================================================================
class GUILabel : public GUIWidget {
public:
    GUILabel(int id, int zIndex, const std::string& text, float x, float y,
             int fontSize = 20, RLColor color = WHITE);

    void draw() override;

    void setText(const std::string& text);
    void setFontSize(int fontSize);
    void setSpacing(int spacing);

    std::string getText() const { return m_text; }
    int getFontSize() const { return m_fontSize; }

private:
    std::string m_text;
    int m_fontSize;
    int m_spacing;
};

#endif // GUI_LABEL_H
