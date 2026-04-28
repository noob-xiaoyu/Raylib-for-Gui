#pragma once
#include <raylib.h>
#include "../backends/gui/gui_widget.h"
#include "../backends/gui/gui_scrollbar.h"
#include <string>
#include <deque>
#include <functional>

#if defined(_WIN32)
// IME 输入法支持（windows.h 在 .cpp 中引入，避免 DrawTextW 宏冲突）
    // 注意：<windows.h> 在 .cpp 中 #include，此处只做前置声明
    void HandleIMEMessage(unsigned int uMsg, uintptr_t wParam, intptr_t lParam);
    static int IMEWindowProc(void* hwnd, unsigned int uMsg, uintptr_t wParam, intptr_t lParam, intptr_t* result, void* user);
#endif

struct TextState {
    std::string text;
    int cursorIndex;
    int selectionAnchor;
};

// TextBox 变种风格
enum class TextBoxStyle {
    Default,        // 默认多行编辑器（原行为）
    SingleLine,     // 单行输入框，Enter 不换行
    Numeric,        // 数字输入框，仅允许数字/小数点/负号
    ReadOnly,       // 只读日志框，自动滚动到底部
    CodeEditor,     // 代码编辑器风格，带行号
    PopupInput      // 浮窗输入（用于 Slider Ctrl+单击数值输入）
};

class TextBox : public GUIWidget {
public:
    TextBox(int id, int zIndex, float x, float y, float width, float height);
    ~TextBox();

    void draw() override;
    void update(float dt) override;
    bool canBeFocused() const override { return true; }
    void onFocus() override;
    void onBlur() override;
    bool containsPoint(float px, float py) const override;

    // 鼠标事件回调（由 GUIManager 统一派发）
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseMove(const MouseState& state) override;
    void onMouseDown(const MouseState& state) override;
    void onMouseUp(const MouseState& state) override;
    void onMouseWheel(const MouseState& state) override;

    void SetText(const std::string& text);
    std::string GetText() const;
    void SetFocus(bool focus);
    bool IsFocused() const;

    void SetFont(float fontSize, float spacing, float lineHeight = 0.0f);
    void SetSize(float width, float height);
    void SetPosition(float x, float y);

    // 变种设置
    void SetStyle(TextBoxStyle style);
    TextBoxStyle GetStyle() const { return m_style; }

    // 密码掩码（SingleLine 模式下）
    void SetPasswordMode(bool enable) { m_passwordMode = enable; }
    bool IsPasswordMode() const { return m_passwordMode; }
    void SetPasswordChar(wchar_t c) { m_passwordChar = c; }

    // 占位符文本
    void SetPlaceholder(const std::string& text) { m_placeholder = text; }
    std::string GetPlaceholder() const { return m_placeholder; }

    // 只读模式附加行（ReadOnly 日志追加）
    void AppendLog(const std::string& line);
    void ClearLog();

    // 数值范围（Numeric 模式）
    void SetNumericRange(double min, double max);
    double GetNumericValue() const;
    void SetNumericValue(double val);
    // 设置数值格式化字符串，默认 "%.6g"，Slider 可设为 "%.2f"
    void SetNumericFormat(const char* fmt) { m_numericFormat = fmt ? fmt : "%.6g"; }

    // 浮窗输入回调（PopupInput 模式）
    void SetPopupCallback(std::function<void(double)> callback);
    void SetPopupLabel(const std::string& label) { m_popupLabel = label; }

    // Enter 键回调（Numeric/SingleLine 模式，回车确认时触发）
    void SetOnEnterCallback(std::function<void()> callback) { m_onEnter = callback; }

    // 行号边距（CodeEditor 模式）
    void SetShowLineNumbers(bool show) { m_showLineNumbers = show; }
    bool IsShowLineNumbers() const { return m_showLineNumbers; }

#if defined(_WIN32)
    // IME 输入法支持
    void HandleIMEMessage(unsigned int uMsg, uintptr_t wParam, intptr_t lParam);
    static int IMEWindowProc(void* hwnd, unsigned int uMsg, uintptr_t wParam, intptr_t lParam, intptr_t* result, void* user);
#endif

private:
    std::deque<TextState> m_undoStack;
    std::deque<TextState> m_redoStack;
    const size_t MAX_HISTORY = 100;
    void SaveUndoState();
    void Undo();
    void Redo();

    void HandleInput();

    void InsertCodepoint(int codepoint);
    void InsertText(const std::string& str);
    void Backspace();
    void Delete();
    void DeleteSelection();
    void Copy();
    void Paste();
    void Cut();
    void PerformAutoIndent();

    void UpdateScrollbarState();
    int GetIndexFromPoint(RLVector2 point);
    RLVector2 GetPositionFromIndex(int index);
    void UpdateIMEPosition();

    int GetPrevCharIndex(int index) const;
    int GetNextCharIndex(int index) const;

    RLRectangle GetTextArea() const;
    bool IsKeyActionTriggered(int key);

    // 变种辅助
    std::string GetDisplayText() const;
    int GetLineNumberWidth() const;
    void DrawLineNumbers(int lineCount, float textAreaX, float textAreaY, float textAreaHeight);
    void DrawPlaceholder(const RLRectangle& textArea);

private:
    std::string m_text;

    float m_fontSize;
    float m_spacing;
    float m_lineHeight;

    ScrollBar m_scrollbar;
    ScrollBar m_hScrollbar; // 水平滚动条

    int m_cursorIndex;
    int m_selectionAnchor;
    int m_lastCursorIndex;

    double m_blinkTimer;
    bool m_isDragging;
    bool m_isHovered;
    RLVector2 m_scrollOffset;

    int m_currentRepeatingKey;
    float m_keyRepeatTimer;
    float m_keyRepeatDelay;
    float m_keyRepeatInterval;

    float m_maxLineWidth; // 当前最长行的像素宽度（水平滚动用）

    bool m_wasHovered;

    // === 变种相关 ===
    TextBoxStyle m_style = TextBoxStyle::Default;
    bool m_passwordMode = false;
    wchar_t m_passwordChar = L'*';
    std::string m_placeholder;

    // Numeric 模式
    double m_numericMin = -1e308;
    double m_numericMax = 1e308;
    std::string m_numericFormat = "%.6g";

    // CodeEditor 模式
    bool m_showLineNumbers = true;

    // PopupInput 模式
    std::string m_popupLabel;
    std::function<void(double)> m_popupCallback;

    // Enter 键回调
    std::function<void()> m_onEnter;

#if defined(_WIN32)
    // IME 输入法状态
    std::wstring m_imeComposition;
    bool m_imeComposing = false;
    bool m_imeConsumeThisFrame = false; // 下一帧跳过 RLGetCharPressed
#endif
};