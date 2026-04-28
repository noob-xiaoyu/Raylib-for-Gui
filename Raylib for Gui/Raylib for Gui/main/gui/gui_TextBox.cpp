#include "gui_TextBox.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include "../backends/font/FontManager.h"
#include "../backends/gui/gui_manager.h"

// windows.h 可能已通过 raylib/GLFW 间接包含，DrawTextW 宏与 FontManager 冲突
#if defined(_WIN32)
#pragma comment(lib, "imm32.lib")
// 保存并取消 NOMINMAX/DrawTextW 等宏以避免冲突
#pragma push_macro("DrawTextW")
#undef DrawTextW
#pragma push_macro("min")
#undef min
#pragma push_macro("max")
#undef max
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <imm.h>
#endif

inline float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

namespace TextUtils {
    std::string CodepointToUTF8(int codepoint) {
        char result[5] = { 0 };
        if (codepoint <= 0x7f) result[0] = (char)codepoint;
        else if (codepoint <= 0x7ff) {
            result[0] = (char)(0xc0 | ((codepoint >> 6) & 0x1f));
            result[1] = (char)(0x80 | (codepoint & 0x3f));
        }
        else if (codepoint <= 0xffff) {
            result[0] = (char)(0xe0 | ((codepoint >> 12) & 0x0f));
            result[1] = (char)(0x80 | ((codepoint >> 6) & 0x3f));
            result[2] = (char)(0x80 | (codepoint & 0x3f));
        }
        else {
            result[0] = (char)(0xf0 | ((codepoint >> 18) & 0x07));
            result[1] = (char)(0x80 | ((codepoint >> 12) & 0x3f));
            result[2] = (char)(0x80 | ((codepoint >> 6) & 0x3f));
            result[3] = (char)(0x80 | (codepoint & 0x3f));
        }
        return std::string(result);
    }

    bool IsCharStart(unsigned char c) {
        return (c & 0xC0) != 0x80;
    }
}

#if defined(_WIN32)
static void* g_imeHookToken = nullptr;
#endif

TextBox::TextBox(int id, int zIndex, float x, float y, float width, float height)
    : GUIWidget(id, zIndex), m_text(""),
    m_cursorIndex(0), m_selectionAnchor(0), m_blinkTimer(0),
    m_isDragging(false), m_scrollOffset({ 0.0f, 0.0f }),
    m_isHovered(false),
    m_style(TextBoxStyle::Default),
    m_passwordMode(false), m_passwordChar(L'*'),
    m_numericMin(-1e308), m_numericMax(1e308),
    m_showLineNumbers(true),
    m_popupCallback(nullptr),
    m_onEnter(nullptr),
    m_hScrollbar(),
    m_maxLineWidth(0)
{
    setPosition(x, y);
    setSize(width, height);
    m_fontSize = 20.0f;
    m_spacing = 1.0f;
    m_lineHeight = m_fontSize;
    m_lastCursorIndex = 0;
    m_currentRepeatingKey = 0;
    m_keyRepeatTimer = 0.0f;
    m_keyRepeatDelay = 0.5f;
    m_keyRepeatInterval = 0.05f;
    m_wasHovered = false;

    m_hScrollbar.setOrientation(ScrollBarOrientation::Horizontal);

#if defined(_WIN32)
    // 注册全局 IME 消息钩子（只需一次）
    if (!g_imeHookToken) {
        g_imeHookToken = RLWin32AddMessageHook(IMEWindowProc, nullptr);
    }
#endif
}

TextBox::~TextBox() {}

void TextBox::SetStyle(TextBoxStyle style) {
    m_style = style;
    if (style == TextBoxStyle::SingleLine || style == TextBoxStyle::Numeric || style == TextBoxStyle::PopupInput) {
        // 单行模式：移除换行符
        for (size_t i = 0; i < m_text.size(); i++) {
            if (m_text[i] == '\n') { m_text.erase(i, 1); i--; }
        }
    }
}

void TextBox::AppendLog(const std::string& line) {
    if (!m_text.empty()) m_text += "\n";
    m_text += line;
    // 自动滚动到底部
    int lineCount = 1;
    for (char c : m_text) if (c == '\n') lineCount++;
    float contentHeight = lineCount * m_lineHeight;
    float maxScroll = contentHeight - m_height;
    if (maxScroll < 0) maxScroll = 0;
    m_scrollOffset.y = -maxScroll;
    m_cursorIndex = (int)m_text.length();
    m_selectionAnchor = m_cursorIndex;
}

void TextBox::ClearLog() {
    m_text.clear();
    m_cursorIndex = 0;
    m_selectionAnchor = 0;
    m_scrollOffset.y = 0;
    m_scrollOffset.x = 0;
}

void TextBox::SetNumericRange(double min, double max) {
    m_numericMin = min;
    m_numericMax = max;
}

double TextBox::GetNumericValue() const {
    try {
        return std::stod(m_text);
    } catch (...) {
        return 0.0;
    }
}

void TextBox::SetNumericValue(double val) {
    val = std::clamp(val, m_numericMin, m_numericMax);
    char buf[64];
    snprintf(buf, sizeof(buf), m_numericFormat.c_str(), val);
    SetText(std::string(buf));
}

void TextBox::SetPopupCallback(std::function<void(double)> callback) {
    m_popupCallback = callback;
}

// 变种辅助：获取显示文本（密码掩码）
std::string TextBox::GetDisplayText() const {
    if (m_passwordMode && !m_text.empty()) {
        return std::string(m_text.length(), (char)m_passwordChar);
    }
    return m_text;
}

// 变种辅助：计算行号宽度
int TextBox::GetLineNumberWidth() const {
    if (!m_showLineNumbers || m_style != TextBoxStyle::CodeEditor) return 0;
    int lineCount = 1;
    for (char c : m_text) if (c == '\n') lineCount++;
    int digits = 1;
    while (lineCount >= 10) { digits++; lineCount /= 10; }
    return digits * (int)m_fontSize + 12;
}

void TextBox::SetSize(float width, float height) {
    setSize(width, height);
}

void TextBox::SetPosition(float x, float y) {
    setPosition(x, y);
}

void TextBox::SetFont(float fontSize, float spacing, float lineHeight) {
    m_fontSize = fontSize;
    m_spacing = spacing;
    m_lineHeight = (lineHeight > 0.0f) ? lineHeight : fontSize;
}

void TextBox::SetText(const std::string& text) {
    m_text = text;
    m_cursorIndex = (int)m_text.length();
    m_selectionAnchor = m_cursorIndex;
}

std::string TextBox::GetText() const { return m_text; }

void TextBox::SetFocus(bool focus) {
    m_focused = focus;
}

bool TextBox::IsFocused() const { return m_focused; }

void TextBox::onFocus() {
    m_focused = true;
}

void TextBox::onBlur() {
    m_focused = false;

#if defined(_WIN32)
    // 失焦时结束 IME 组合
    if (m_imeComposing) {
        m_imeComposing = false;
        m_imeComposition.clear();
        HWND hwnd = (HWND)RLGetWindowHandle();
        HIMC himc = ImmGetContext(hwnd);
        if (himc) {
            ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
            ImmReleaseContext(hwnd, himc);
        }
    }
#endif

    // PopupInput 风格：失去焦点时自动触发回调
    if (m_style == TextBoxStyle::PopupInput && m_popupCallback) {
        m_popupCallback(GetNumericValue());
    }
}

void TextBox::update(float dt) {
    if (!m_visible || !m_enabled) return;

    const MouseState& ms = GUIManager::GetInstance().getMouseState();
    bool onExpandedDropdown = GUIManager::GetInstance().isPointOnExpandedDropdown(ms.position.x, ms.position.y);

    if (!onExpandedDropdown) {
        UpdateScrollbarState();
    }

    if (!m_focused) return;

    if (!m_scrollbar.isDragging() && !m_hScrollbar.isDragging()) {
        HandleInput();
        if (m_cursorIndex != m_lastCursorIndex) {
            RLRectangle textArea = GetTextArea();
            RLVector2 cPos = GetPositionFromIndex(m_cursorIndex);

            // 垂直滚动
            float relativeY = cPos.y - textArea.y;
            if (relativeY < 0) {
                m_scrollOffset.y -= relativeY;
            }
            else if (relativeY + m_lineHeight > textArea.height) {
                m_scrollOffset.y -= (relativeY + m_lineHeight - textArea.height);
            }

            // 水平滚动
            float relativeX = cPos.x - textArea.x;
            if (relativeX < 0) {
                m_scrollOffset.x -= relativeX;
            }
            else if (relativeX > textArea.width) {
                m_scrollOffset.x -= (relativeX - textArea.width);
            }

            UpdateScrollbarState();
            m_lastCursorIndex = m_cursorIndex;
        }
        m_blinkTimer += dt;
    }

    // IME 输入法期间每帧更新候选窗口位置
#if defined(_WIN32)
    if (m_imeComposing) {
        UpdateIMEPosition();
    }
#endif
}

bool TextBox::IsKeyActionTriggered(int key) {
    if (RLIsKeyPressed(key)) {
        m_currentRepeatingKey = key;
        m_keyRepeatTimer = 0.0f;
        return true;
    }

    if (RLIsKeyDown(key) && m_currentRepeatingKey == key) {
        m_keyRepeatTimer += RLGetFrameTime();

        if (m_keyRepeatTimer > m_keyRepeatDelay) {
            if (m_keyRepeatTimer > m_keyRepeatDelay + m_keyRepeatInterval) {
                m_keyRepeatTimer -= m_keyRepeatInterval;
                return true;
            }
        }
    }

    if (RLIsKeyReleased(key)) {
        if (m_currentRepeatingKey == key) m_currentRepeatingKey = 0;
    }

    return false;
}

bool TextBox::containsPoint(float px, float py) const {
    auto pos = getAbsolutePosition();
    return px >= pos.x && px <= pos.x + m_width &&
           py >= pos.y && py <= pos.y + m_height;
}

// ============================================================================
// IME 输入法支持（仅 Windows）
// ============================================================================
#if defined(_WIN32)
void TextBox::HandleIMEMessage(unsigned int uMsg, uintptr_t wParam, intptr_t lParam) {
    if (!m_focused) return;

    HWND hwnd = (HWND)RLGetWindowHandle();
    HIMC himc = ImmGetContext(hwnd);
    if (!himc) return;

    if (uMsg == WM_IME_STARTCOMPOSITION) {
        m_imeComposing = true;
        m_imeComposition.clear();
        m_imeConsumeThisFrame = false;
    }
    else if (uMsg == WM_IME_COMPOSITION) {
        if (lParam & GCS_COMPSTR) {
            // 正在编辑中（输入法已经输入部分文字）
            LONG len = ImmGetCompositionStringW(himc, GCS_COMPSTR, nullptr, 0);
            if (len > 0) {
                std::wstring buf(len / 2 + 1, L'\0');
                ImmGetCompositionStringW(himc, GCS_COMPSTR, &buf[0], len);
                buf.resize(len / 2);
                m_imeComposition = buf;
            } else {
                m_imeComposition.clear();
            }
        }
        if (lParam & GCS_RESULTSTR) {
            // 输入法确认文字
            LONG len = ImmGetCompositionStringW(himc, GCS_RESULTSTR, nullptr, 0);
            if (len > 0) {
                std::wstring buf(len / 2 + 1, L'\0');
                ImmGetCompositionStringW(himc, GCS_RESULTSTR, &buf[0], len);
                buf.resize(len / 2);
                m_imeComposition.clear();
                // 转换宽字符到 UTF-8 并插入
                int utf8Len = WideCharToMultiByte(CP_UTF8, 0, buf.c_str(), (int)buf.length(), nullptr, 0, nullptr, nullptr);
                if (utf8Len > 0) {
                    std::string utf8(utf8Len, '\0');
                    WideCharToMultiByte(CP_UTF8, 0, buf.c_str(), (int)buf.length(), &utf8[0], utf8Len, nullptr, nullptr);
                    DeleteSelection();
                    InsertText(utf8);
                }
            }
        }
    }
    else if (uMsg == WM_IME_ENDCOMPOSITION) {
        m_imeComposing = false;
        m_imeComposition.clear();
        // 标记下一帧跳过 RLGetCharPressed，防止 GLFW 翻译的 WM_CHAR 重复输入
        m_imeConsumeThisFrame = true;
    }

    ImmReleaseContext(hwnd, himc);
}

int TextBox::IMEWindowProc(void* hwnd, unsigned int uMsg, uintptr_t wParam, intptr_t lParam, intptr_t* result, void* user) {
    (void)user;
    if (uMsg == WM_IME_STARTCOMPOSITION || uMsg == WM_IME_COMPOSITION || uMsg == WM_IME_ENDCOMPOSITION) {
        HWND hWnd = (HWND)hwnd;

        // 1. 先调用 DefWindowProc，让系统 IME 管理器正常处理消息、管理内部状态
        //    必须调用，否则 ImmSetCandidateWindow 等 API 不会生效
        LRESULT defResult = DefWindowProcW(hWnd, uMsg, wParam, lParam);

        // 2. 再处理 IME 消息（读取组合字符串、插入确认文字）
        GUIManager& gui = GUIManager::GetInstance();
        GUIWidget* focused = gui.getFocusedWidget();
        if (focused) {
            TextBox* tb = dynamic_cast<TextBox*>(focused);
            if (tb) {
                tb->HandleIMEMessage(uMsg, wParam, lParam);
            }
        }

        // 3. 强制更新 IME 窗口位置到光标处（覆盖 DefWindowProc 可能设置的默认位置）
        if (focused) {
            TextBox* tb = dynamic_cast<TextBox*>(focused);
            if (tb) {
                tb->UpdateIMEPosition();
            }
        }

        *result = (intptr_t)defResult;
        return 1; // 已处理，阻止 GLFW 二次处理
    }
    return 0; // 未处理，继续传递
}
#endif

void TextBox::HandleInput() {
    // ReadOnly 模式不接受任何输入
    if (m_style == TextBoxStyle::ReadOnly) return;

    // IME 组合期间或 IME 结束后的下一帧跳过 RLGetCharPressed（由 IME 消息单独处理）
#if defined(_WIN32)
    if (!m_imeComposing && !m_imeConsumeThisFrame)
#else
    if (true)
#endif
    {
        int key = RLGetCharPressed();
        while (key > 0) {
            if (key >= 32) {
                // Numeric 模式：仅允许数字、小数点、负号
                if (m_style == TextBoxStyle::Numeric) {
                    bool valid = (key >= '0' && key <= '9') || key == '.' || key == '-';
                    if (valid) {
                        DeleteSelection();
                        InsertCodepoint(key);
                    }
                } else {
                    DeleteSelection();
                    InsertCodepoint(key);
                }
            }
            key = RLGetCharPressed();
        }
    }
#if defined(_WIN32)
    m_imeConsumeThisFrame = false;
#endif

    bool ctrl = RLIsKeyDown(RL_E_KEY_LEFT_CONTROL) || RLIsKeyDown(RL_E_KEY_RIGHT_CONTROL);
    bool shift = RLIsKeyDown(RL_E_KEY_LEFT_SHIFT) || RLIsKeyDown(RL_E_KEY_RIGHT_SHIFT);

    if (ctrl) {
        if (RLIsKeyPressed(RL_E_KEY_C)) Copy();
        else if (IsKeyActionTriggered(RL_E_KEY_V)) { SaveUndoState(); Paste(); }
        else if (IsKeyActionTriggered(RL_E_KEY_X)) { SaveUndoState(); Cut(); }
        else if (RLIsKeyPressed(RL_E_KEY_A)) {
            m_selectionAnchor = 0;
            m_cursorIndex = (int)m_text.length();
        }
        else if (IsKeyActionTriggered(RL_E_KEY_Z)) {
            if (shift) Redo();
            else Undo();
        }
        else if (IsKeyActionTriggered(RL_E_KEY_Y)) {
            Redo();
        }
    }

    // SingleLine / Numeric / PopupInput 模式：Enter 触发回调（如果是 PopupInput）
    bool isSingleLine = (m_style == TextBoxStyle::SingleLine ||
                         m_style == TextBoxStyle::Numeric ||
                         m_style == TextBoxStyle::PopupInput);

    if (IsKeyActionTriggered(RL_E_KEY_ENTER)) {
        if (isSingleLine) {
            // 通用 Enter 回调（Slider 输入框提交等）
            if (m_onEnter) {
                m_onEnter();
            }
            // PopupInput: Enter 确认数值
            if (m_style == TextBoxStyle::PopupInput && m_popupCallback) {
                m_popupCallback(GetNumericValue());
            }
            return;
        }
        SaveUndoState();
        DeleteSelection();
        InsertText("\n");
        PerformAutoIndent();
    }
    else if (IsKeyActionTriggered(RL_E_KEY_BACKSPACE)) {
        if (m_cursorIndex != m_selectionAnchor) DeleteSelection();
        else Backspace();
    }
    else if (IsKeyActionTriggered(RL_E_KEY_DELETE)) {
        if (m_cursorIndex != m_selectionAnchor) DeleteSelection();
        else Delete();
    }

    // SingleLine 模式：禁止上下键移动光标
    if (IsKeyActionTriggered(RL_E_KEY_LEFT)) {
        m_cursorIndex = GetPrevCharIndex(m_cursorIndex);
        if (!shift) m_selectionAnchor = m_cursorIndex;
        m_blinkTimer = 0;
    }
    else if (IsKeyActionTriggered(RL_E_KEY_RIGHT)) {
        m_cursorIndex = GetNextCharIndex(m_cursorIndex);
        if (!shift) m_selectionAnchor = m_cursorIndex;
        m_blinkTimer = 0;
    }
    else if (!isSingleLine && IsKeyActionTriggered(RL_E_KEY_UP)) {
        RLVector2 currentPos = GetPositionFromIndex(m_cursorIndex);
        currentPos.y -= m_lineHeight;
        m_cursorIndex = GetIndexFromPoint(currentPos);
        if (!shift) m_selectionAnchor = m_cursorIndex;
        m_blinkTimer = 0;
    }
    else if (!isSingleLine && IsKeyActionTriggered(RL_E_KEY_DOWN)) {
        RLVector2 currentPos = GetPositionFromIndex(m_cursorIndex);
        currentPos.y += m_lineHeight;
        m_cursorIndex = GetIndexFromPoint(currentPos);
        if (!shift) m_selectionAnchor = m_cursorIndex;
        m_blinkTimer = 0;
    }
    else if (IsKeyActionTriggered(RL_E_KEY_HOME)) {
        // 移动到行首
        int lineStart = m_cursorIndex;
        if (lineStart > 0) {
            int prevNL = (int)m_text.rfind('\n', lineStart - 1);
            lineStart = (prevNL == std::string::npos) ? 0 : prevNL + 1;
        }
        m_cursorIndex = lineStart;
        if (!shift) m_selectionAnchor = m_cursorIndex;
        m_blinkTimer = 0;
    }
    else if (IsKeyActionTriggered(RL_E_KEY_END)) {
        // 移动到行尾
        int lineEnd = (int)m_text.find('\n', m_cursorIndex);
        if (lineEnd == std::string::npos) lineEnd = (int)m_text.length();
        m_cursorIndex = lineEnd;
        if (!shift) m_selectionAnchor = m_cursorIndex;
        m_blinkTimer = 0;
    }
}

// ============================================================================
// 鼠标事件回调（由 GUIManager 统一派发）
// ============================================================================

void TextBox::onMouseEnter() {
    m_isHovered = true;
    m_wasHovered = true;
    // 进入时根据鼠标位置设置正确光标，避免在滚动条上误设 I-beam
    RLVector2 mp = GUIManager::GetInstance().getMouseState().position;
    bool onScrollbar = m_scrollbar.isVisible() && RLCheckCollisionPointRec(mp, m_scrollbar.getRect());
    bool onHScrollbar = m_hScrollbar.isVisible() && RLCheckCollisionPointRec(mp, m_hScrollbar.getRect());
    RLSetMouseCursor((onScrollbar || onHScrollbar) ? RL_E_MOUSE_CURSOR_DEFAULT : RL_E_MOUSE_CURSOR_IBEAM);
}

void TextBox::onMouseLeave() {
    m_isHovered = false;
    RLSetMouseCursor(RL_E_MOUSE_CURSOR_DEFAULT);
    m_wasHovered = false;
}

void TextBox::onMouseMove(const MouseState& state) {
    // 拖拽中更新选区
    if (m_isDragging && !m_scrollbar.isDragging() && !m_hScrollbar.isDragging()) {
        m_cursorIndex = GetIndexFromPoint(state.position);
    }

    // 根据鼠标位置动态切换光标
    // 鼠标不在控件范围内 → 默认光标（处理拖拽移出场景）
    if (!containsPoint(state.position.x, state.position.y)) {
        RLSetMouseCursor(RL_E_MOUSE_CURSOR_DEFAULT);
        return;
    }

    RLVector2 pos = state.position;
    bool onScrollbar = m_scrollbar.isVisible() && RLCheckCollisionPointRec(pos, m_scrollbar.getRect());
    bool onHScrollbar = m_hScrollbar.isVisible() && RLCheckCollisionPointRec(pos, m_hScrollbar.getRect());
    if (onScrollbar || onHScrollbar || m_scrollbar.isDragging() || m_hScrollbar.isDragging()) {
        RLSetMouseCursor(RL_E_MOUSE_CURSOR_DEFAULT);
    } else {
        RLSetMouseCursor(RL_E_MOUSE_CURSOR_IBEAM);
    }
}

void TextBox::onMouseDown(const MouseState& state) {
    // 直接使用滚动条自身的矩形检测点击，保证与 UpdateScrollbarState 中设置的 rect 一致
    bool onScrollbar = m_scrollbar.isVisible() &&
        RLCheckCollisionPointRec(state.position, m_scrollbar.getRect());
    bool onHScrollbar = m_hScrollbar.isVisible() &&
        RLCheckCollisionPointRec(state.position, m_hScrollbar.getRect());

    bool onExpandedDropdown = GUIManager::GetInstance().isPointOnExpandedDropdown(
        state.position.x, state.position.y);

    if (!onScrollbar && !onHScrollbar && !onExpandedDropdown) {
        GUIManager::GetInstance().setFocus(m_id);
        m_isDragging = true;
        m_lastCursorIndex = -1;
        m_cursorIndex = GetIndexFromPoint(state.position);
        m_selectionAnchor = m_cursorIndex;
    }
}

void TextBox::onMouseWheel(const MouseState& state) {
    int lineCount = 1;
    for (char c : m_text) if (c == '\n') lineCount++;
    float contentHeight = lineCount * m_lineHeight;

    auto pos = getAbsolutePosition();
    RLRectangle textArea = { pos.x, pos.y, m_width - (m_scrollbar.isVisible() ? 12 : 0), m_height - (m_hScrollbar.isVisible() ? 8 : 0) };

    if (!RLCheckCollisionPointRec(state.position, textArea) &&
        !(m_scrollbar.isVisible() && RLCheckCollisionPointRec(state.position, m_scrollbar.getRect())) &&
        !(m_hScrollbar.isVisible() && RLCheckCollisionPointRec(state.position, m_hScrollbar.getRect()))) {
        return;
    }

    bool shift = RLIsKeyDown(RL_E_KEY_LEFT_SHIFT) || RLIsKeyDown(RL_E_KEY_RIGHT_SHIFT);

    // 水平滚轮（触摸板横向滑动 / 鼠标左右倾斜滚轮）
    if (state.wheelMoveH != 0.0f) {
        m_scrollOffset.x += state.wheelMoveH * m_fontSize * 3;
        float maxScroll = (m_maxLineWidth > textArea.width) ? (m_maxLineWidth - textArea.width) : 0;
        if (m_scrollOffset.x > 0) m_scrollOffset.x = 0;
        if (m_scrollOffset.x < -maxScroll) m_scrollOffset.x = -maxScroll;
    }
    else if (shift) {
        // Shift + 滚轮 → 水平滚动
        m_scrollOffset.x += state.wheelMove * m_fontSize * 3;
        float maxScroll = (m_maxLineWidth > textArea.width) ? (m_maxLineWidth - textArea.width) : 0;
        if (m_scrollOffset.x > 0) m_scrollOffset.x = 0;
        if (m_scrollOffset.x < -maxScroll) m_scrollOffset.x = -maxScroll;
    }
    else {
        // 默认垂直滚动
        m_scrollOffset.y += state.wheelMove * m_lineHeight * 3;
        float maxScroll = (contentHeight > textArea.height) ? (contentHeight - textArea.height) : 0;
        if (m_scrollOffset.y > 0) m_scrollOffset.y = 0;
        if (m_scrollOffset.y < -maxScroll) m_scrollOffset.y = -maxScroll;
    }
    UpdateScrollbarState();
}

void TextBox::onMouseUp(const MouseState& state) {
    m_isDragging = false;
}

void TextBox::InsertCodepoint(int codepoint) {
    InsertText(TextUtils::CodepointToUTF8(codepoint));
}

void TextBox::InsertText(const std::string& str) {
    SaveUndoState();
    m_text.insert(m_cursorIndex, str);
    m_cursorIndex += (int)str.length();
    m_selectionAnchor = m_cursorIndex;
}

void TextBox::Backspace() {
    if (m_cursorIndex > 0) {
        SaveUndoState();
        int prev = GetPrevCharIndex(m_cursorIndex);
        m_text.erase(prev, m_cursorIndex - prev);
        m_cursorIndex = prev;
        m_selectionAnchor = m_cursorIndex;
    }
}

void TextBox::Delete() {
    if (m_cursorIndex < (int)m_text.length()) {
        SaveUndoState();
        int next = GetNextCharIndex(m_cursorIndex);
        m_text.erase(m_cursorIndex, next - m_cursorIndex);
    }
}

void TextBox::DeleteSelection() {
    if (m_cursorIndex == m_selectionAnchor) return;
    SaveUndoState();
    int start = std::min(m_cursorIndex, m_selectionAnchor);
    int end = std::max(m_cursorIndex, m_selectionAnchor);
    m_text.erase(start, end - start);
    m_cursorIndex = start;
    m_selectionAnchor = start;
}

void TextBox::PerformAutoIndent() {
    if (m_cursorIndex <= 1) return;
    int prevLineEnd = m_cursorIndex - 1;
    int prevLineStart = (int)m_text.rfind('\n', prevLineEnd - 1);

    if (prevLineStart == std::string::npos) prevLineStart = 0;
    else prevLineStart++;

    std::string indent = "";
    for (int i = prevLineStart; i < prevLineEnd; i++) {
        char c = m_text[i];
        if (c == ' ' || c == '\t') indent += c;
        else break;
    }
    if (!indent.empty()) InsertText(indent);
}

void TextBox::SaveUndoState() {
    if (!m_undoStack.empty() && m_undoStack.back().text == m_text) {
        return;
    }

    m_undoStack.push_back({ m_text, m_cursorIndex, m_selectionAnchor });

    if (m_undoStack.size() > MAX_HISTORY) {
        m_undoStack.pop_front();
    }

    m_redoStack.clear();
}

void TextBox::Undo() {
    if (m_undoStack.empty()) return;

    m_redoStack.push_back({ m_text, m_cursorIndex, m_selectionAnchor });

    TextState last = m_undoStack.back();
    m_undoStack.pop_back();

    m_text = last.text;
    m_cursorIndex = last.cursorIndex;
    m_selectionAnchor = last.selectionAnchor;

    m_lastCursorIndex = -1;
}

void TextBox::Redo() {
    if (m_redoStack.empty()) return;

    m_undoStack.push_back({ m_text, m_cursorIndex, m_selectionAnchor });

    TextState next = m_redoStack.back();
    m_redoStack.pop_back();

    m_text = next.text;
    m_cursorIndex = next.cursorIndex;
    m_selectionAnchor = next.selectionAnchor;

    m_lastCursorIndex = -1;
}

void TextBox::Copy() {
    if (m_cursorIndex != m_selectionAnchor) {
        int start = std::min(m_cursorIndex, m_selectionAnchor);
        int end = std::max(m_cursorIndex, m_selectionAnchor);
        RLSetClipboardText(m_text.substr(start, end - start).c_str());
    }
}

void TextBox::Paste() {
    const char* clip = RLGetClipboardText();
    if (clip) {
        DeleteSelection();
        InsertText(clip);
    }
}

void TextBox::Cut() {
    Copy();
    DeleteSelection();
}

int TextBox::GetIndexFromPoint(RLVector2 point) {
    RLRectangle textArea = GetTextArea();
    point.x -= textArea.x + m_scrollOffset.x;
    point.y -= textArea.y + m_scrollOffset.y;

    int line = (int)(point.y / m_lineHeight);
    if (line < 0) line = 0;

    int currentLine = 0;
    int idx = 0;
    int lastLineStart = 0;

    while (idx < (int)m_text.length()) {
        if (m_text[idx] == '\n') {
            if (currentLine == line) break;
            currentLine++;
            idx++;
            lastLineStart = idx;
        }
        else {
            idx++;
        }
    }

    if (currentLine < line) return (int)m_text.length();

    int lineEnd = idx;

    for (int i = lastLineStart; i < lineEnd; ) {
        int nextI = GetNextCharIndex(i);

        std::string subStr = m_text.substr(lastLineStart, nextI - lastLineStart);
        float widthToEnd = MeasureTextCustom(subStr.c_str(), (int)m_fontSize, (int)m_spacing).x;

        std::string charStr = m_text.substr(i, nextI - i);
        float charWidth = MeasureTextCustom(charStr.c_str(), (int)m_fontSize, (int)m_spacing).x;

        float charLeftX = widthToEnd - charWidth;

        if (point.x < charLeftX + charWidth / 2) {
            return i;
        }

        i = nextI;
    }

    return lineEnd;
}

RLVector2 TextBox::GetPositionFromIndex(int index) {
    RLRectangle textArea = GetTextArea();
    if (index > (int)m_text.length()) index = (int)m_text.length();

    int line = 0;
    int lineStart = 0;
    for (int i = 0; i < index; i++) {
        if (m_text[i] == '\n') {
            line++;
            lineStart = i + 1;
        }
    }

    std::string sub = m_text.substr(lineStart, index - lineStart);
    RLVector2 size = MeasureTextCustom(sub.c_str(), (int)m_fontSize, (int)m_spacing);

    return RLVector2{
        textArea.x + m_scrollOffset.x + size.x,
        textArea.y + m_scrollOffset.y + line * m_lineHeight
    };
}

void TextBox::UpdateIMEPosition() {
#if defined(_WIN32)
    if (m_focused) {
        HWND hwnd = (HWND)RLGetWindowHandle();
        HIMC himc = ImmGetContext(hwnd);
        if (himc) {
            RLVector2 localPos = GetPositionFromIndex(m_cursorIndex);

            // 将客户区坐标转换为屏幕坐标
            POINT pt;
            pt.x = (LONG)localPos.x;
            pt.y = (LONG)(localPos.y + m_lineHeight);
            ClientToScreen(hwnd, &pt);

            COMPOSITIONFORM cf = {};
            cf.dwStyle = CFS_POINT;
            cf.ptCurrentPos.x = pt.x;
            cf.ptCurrentPos.y = pt.y;
            ImmSetCompositionWindow(himc, &cf);

            CANDIDATEFORM cand = {};
            cand.dwStyle = CFS_CANDIDATEPOS;
            cand.ptCurrentPos.x = pt.x;
            cand.ptCurrentPos.y = pt.y;
            ImmSetCandidateWindow(himc, &cand);

            ImmReleaseContext(hwnd, himc);
        }
    }
#endif
}

void TextBox::UpdateScrollbarState() {
    int lineCount = 1;
    for (char c : m_text) if (c == '\n') lineCount++;
    float contentHeight = lineCount * m_lineHeight;

    auto pos = getAbsolutePosition();

    bool singleLineMode = (m_style == TextBoxStyle::SingleLine ||
                           m_style == TextBoxStyle::Numeric ||
                           m_style == TextBoxStyle::PopupInput);

    // 单行模式下不显示水平滚动条
    float hBarHeight = (!singleLineMode && m_hScrollbar.isVisible()) ? 7 : 0;

    // 垂直滚动条
    m_scrollbar.setRect(pos.x + m_width - 8, pos.y + 1, 12-5, m_height - hBarHeight - 2);
    m_scrollbar.setContent(contentHeight, m_height - hBarHeight);

    m_scrollbar.setScrollOffset(m_scrollOffset.y);

    const MouseState& ms = GUIManager::GetInstance().getMouseState();
    m_scrollbar.handleScroll(ms);

    m_scrollOffset.y = m_scrollbar.getScrollOffset();

    // === 水平滚动条 ===
    m_maxLineWidth = 0;
    int lineStart = 0;
    for (int i = 0; i <= (int)m_text.length(); i++) {
        if (i == (int)m_text.length() || m_text[i] == '\n') {
            std::string lineStr = m_text.substr(lineStart, i - lineStart);
            float lineWidth = MeasureTextCustom(lineStr.c_str(), (int)m_fontSize, (int)m_spacing).x;
            if (lineWidth > m_maxLineWidth) m_maxLineWidth = lineWidth;
            lineStart = i + 1;
        }
    }

    RLRectangle textArea = GetTextArea();

    if (!singleLineMode) {
        m_hScrollbar.setRect(pos.x+1, pos.y + m_height - 8, m_width - (m_scrollbar.isVisible() ? 9 : 0), 7);
        m_hScrollbar.setContent(m_maxLineWidth, textArea.width);
        m_hScrollbar.setScrollOffset(m_scrollOffset.x);
        m_hScrollbar.handleScroll(ms);
        m_scrollOffset.x = m_hScrollbar.getScrollOffset();
    }
}

int TextBox::GetPrevCharIndex(int index) const {
    if (index <= 0) return 0;
    int i = index - 1;
    while (i > 0 && !TextUtils::IsCharStart((unsigned char)m_text[i])) i--;
    return i;
}

int TextBox::GetNextCharIndex(int index) const {
    if (index >= (int)m_text.length()) return (int)m_text.length();
    int i = index + 1;
    while (i < (int)m_text.length() && !TextUtils::IsCharStart((unsigned char)m_text[i])) i++;
    return i;
}

RLRectangle TextBox::GetTextArea() const {
    auto pos = getAbsolutePosition();
    RLRectangle textArea = { pos.x, pos.y, m_width, m_height };
    if (m_scrollbar.isVisible()) {
        textArea.width -= 12;
    }
    // 单行模式不显示水平滚动条（不占底部空间）
    bool singleLineMode = (m_style == TextBoxStyle::SingleLine ||
                           m_style == TextBoxStyle::Numeric ||
                           m_style == TextBoxStyle::PopupInput);
    if (!singleLineMode && m_hScrollbar.isVisible()) {
        textArea.height -= 8;
    }
    // 代码编辑器模式留出行号空间
    if (m_style == TextBoxStyle::CodeEditor && m_showLineNumbers) {
        textArea.x += GetLineNumberWidth();
        textArea.width -= GetLineNumberWidth();
    }
    // 自适应 padding：高度小时自动收窄
    float padding = 4.0f;
    if (m_height < 30.0f) padding = 2.0f;
    if (m_height < 22.0f) padding = 1.0f;
    textArea.x += padding;
    textArea.y += padding;
    textArea.width -= padding * 2;
    textArea.height -= padding * 2;

    // 确保文本区域至少能显示一行
    if (textArea.height < m_lineHeight) {
        textArea.height = m_lineHeight;
    }

    return textArea;
}

void TextBox::draw() {
    if (!m_visible) return;

    auto pos = getAbsolutePosition();
    RLRectangle bounds = { pos.x, pos.y, m_width, m_height };
    RLRectangle textArea = GetTextArea();

    // 变种颜色
    RLColor bgColor = DARKGRAY;
    RLColor textColor = RAYWHITE;
    RLColor borderColor = m_focused ? SKYBLUE : GRAY;

    if (m_style == TextBoxStyle::CodeEditor) {
        bgColor = RLColor{ 30, 30, 40, 255 };
    } else if (m_style == TextBoxStyle::ReadOnly) {
        bgColor = RLColor{ 25, 25, 30, 255 };
    } else if (m_style == TextBoxStyle::PopupInput) {
        // 浮窗输入：半透明背景
        bgColor = RLColor{ 40, 40, 60, 240 };
    }

    // 背景
    RLDrawRectangleRec(bounds, bgColor);
    RLDrawRectangleLinesEx(bounds, 1, borderColor);

    // 代码编辑器：行号背景
    if (m_style == TextBoxStyle::CodeEditor && m_showLineNumbers) {
        float lineNumWidth = (float)GetLineNumberWidth();
        RLRectangle lnBg{ pos.x, pos.y, lineNumWidth, m_height };
        RLDrawRectangleRec(lnBg, RLColor{ 25, 25, 35, 255 });
        // 行号与文本区的分隔线
        RLDrawLineEx({ pos.x + lineNumWidth, pos.y }, { pos.x + lineNumWidth, pos.y + m_height },
                     1, RLColor{ 60, 60, 80, 255 });
    }

    // 占位符文本（当文本框为空且有占位符时）
    if (m_text.empty() && !m_placeholder.empty() && !m_focused) {
        DrawPlaceholder(textArea);
        // 即使有占位符，也继续绘制滚动条
        if (m_scrollbar.isVisible() && m_style != TextBoxStyle::SingleLine &&
            m_style != TextBoxStyle::Numeric && m_style != TextBoxStyle::PopupInput) {
            m_scrollbar.draw();
        }
        if (m_hScrollbar.isVisible() && m_style != TextBoxStyle::SingleLine &&
            m_style != TextBoxStyle::Numeric && m_style != TextBoxStyle::PopupInput) {
            m_hScrollbar.draw();
        }
        return;
    }

    // 行号在 scissor 外部绘制，避免被文本裁剪区域切掉
    if (m_style == TextBoxStyle::CodeEditor && m_showLineNumbers) {
        int lineCount = 1;
        for (char c : m_text) if (c == '\n') lineCount++;
        DrawLineNumbers(lineCount, textArea.x, textArea.y, textArea.height);
    }

    RLBeginScissorMode((int)textArea.x, (int)textArea.y, (int)textArea.width, (int)textArea.height);

    std::string displayText = GetDisplayText();

    // 选择区域高亮
    if (m_cursorIndex != m_selectionAnchor) {
        int start = std::min(m_cursorIndex, m_selectionAnchor);
        int end = std::max(m_cursorIndex, m_selectionAnchor);
        int p = start;
        while (p < end) {
            size_t nextNL = displayText.find('\n', p);
            int lineEnd = (nextNL == std::string::npos || nextNL >= (size_t)end) ? end : (int)nextNL;

            RLVector2 posStart = GetPositionFromIndex(p);
            float width = MeasureTextCustom(displayText.substr(p, lineEnd - p).c_str(), (int)m_fontSize, (int)m_spacing).x;
            if (width < 2.0f && lineEnd < (int)displayText.length() && displayText[lineEnd] == '\n') width = 8.0f;

            if (posStart.y + m_lineHeight > pos.y && posStart.y < pos.y + m_height) {
                RLDrawRectangleRec({ posStart.x, posStart.y, width, m_lineHeight }, RLColor{ 0, 120, 255, 100 });
            }
            p = lineEnd;
            if (p < end && displayText[p] == '\n') p++;
        }
    }

    // 绘制文本行
    int start = 0;
    int lineCount = 0;
    for (int i = 0; i <= (int)displayText.length(); i++) {
        if (i == (int)displayText.length() || displayText[i] == '\n') {
            std::string lineStr = displayText.substr(start, i - start);
            RLVector2 drawPos = {
                textArea.x + m_scrollOffset.x,
                textArea.y + m_scrollOffset.y + lineCount * m_lineHeight
            };
            if (drawPos.y + m_lineHeight > textArea.y && drawPos.y < textArea.y + textArea.height) {
                DrawTextCustom(lineStr.c_str(), drawPos, (int)m_fontSize, textColor, (int)m_spacing);
            }
            lineCount++;
            start = i + 1;
        }
    }

    // 光标闪烁
    if (m_focused && ((int)(m_blinkTimer * 2) % 2 == 0)) {
        RLVector2 cPos = GetPositionFromIndex(m_cursorIndex);
        float cursorHeight = (m_style == TextBoxStyle::CodeEditor) ? m_lineHeight : m_lineHeight;
        RLDrawLineEx(cPos, RLVector2{ cPos.x, cPos.y + cursorHeight }, 2, RED);
    }

    // IME 组合字符串内联显示（在光标位置绘制下划线文本）
#if defined(_WIN32)
    if (m_focused && m_imeComposing && !m_imeComposition.empty()) {
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, m_imeComposition.c_str(), (int)m_imeComposition.length(), nullptr, 0, nullptr, nullptr);
        if (utf8Len > 0) {
            std::string utf8(utf8Len, '\0');
            WideCharToMultiByte(CP_UTF8, 0, m_imeComposition.c_str(), (int)m_imeComposition.length(), &utf8[0], utf8Len, nullptr, nullptr);
            RLVector2 cPos = GetPositionFromIndex(m_cursorIndex);
            // 绘制组合文本
            DrawTextCustom(utf8.c_str(), cPos, (int)m_fontSize, RLColor{ 200, 220, 255, 255 }, (int)m_spacing);
            // 绘制下划线
            RLVector2 textSize = MeasureTextCustom(utf8.c_str(), (int)m_fontSize, (int)m_spacing);
            float underlineY = cPos.y + m_lineHeight - 2;
            RLDrawLineEx(cPos, RLVector2{ cPos.x + textSize.x, underlineY }, 2, RLColor{ 200, 220, 255, 160 });
        }
    }
#endif

    RLEndScissorMode();

    // 滚动条
    m_scrollbar.draw();
    m_hScrollbar.draw();
}

// 变种辅助：绘制行号
void TextBox::DrawLineNumbers(int lineCount, float textAreaX, float textAreaY, float textAreaHeight) {
    float lnWidth = (float)GetLineNumberWidth();
    auto pos = getAbsolutePosition();

    for (int i = 0; i < lineCount; i++) {
        float y = textAreaY + m_scrollOffset.y + i * m_lineHeight;
        if (y + m_lineHeight > textAreaY && y < textAreaY + textAreaHeight) {
            std::string num = std::to_string(i + 1);
            RLVector2 size = MeasureTextCustom(num.c_str(), (int)m_fontSize, (int)m_spacing);
            float nx = pos.x + lnWidth - size.x - 6;
            DrawTextCustom(num.c_str(), { nx, y }, (int)m_fontSize, RLColor{ 120, 120, 160, 255 }, (int)m_spacing);
        }
    }
}

// 变种辅助：绘制占位符
void TextBox::DrawPlaceholder(const RLRectangle& textArea) {
    RLColor placeholderColor{ 120, 120, 140, 200 };
    RLVector2 drawPos = { textArea.x, textArea.y };
    DrawTextCustom(m_placeholder.c_str(), drawPos, (int)m_fontSize, placeholderColor, (int)m_spacing);
}

