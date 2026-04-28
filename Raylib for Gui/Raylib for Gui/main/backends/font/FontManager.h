#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include "raylib.h"

// ============================================================================
// FontManager - 单例字体管理器
// 封装字体加载、重建、文本绘制等所有字体相关操作
// ============================================================================
class FontManager {
public:
    static FontManager& GetInstance();

    // 初始化加载字体（搜索 font/ 目录 → 系统字体）
    void LoadCustomFont();

    // 确保文本中的字符已加载，必要时重建字体
    void EnsureFont(const char* text);

    // 绘制文本
    void DrawText(const char* text, RLVector2 pos, int fontSize, RLColor color, int spacing = 2);

    // 测量文本尺寸
    RLVector2 MeasureText(const char* text, int fontSize, int spacing = 2);

    // 获取 raylib 字体引用
    const RLFont& GetFont() const { return m_font; }

    // 释放字体资源
    void Unload();

private:
    FontManager() = default;
    ~FontManager() = default;
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    std::vector<int> ExtractCodepoints(const char* text);
    void RebuildFont();

    RLFont m_font = { 0 };
    std::string m_fontPath;
    int m_fontSizeGlobal = 32;
    std::unordered_set<int> m_codepointSet;
    std::vector<int> m_codepoints;
    bool m_usingDefaultFont = false;
};

// ============================================================================
// 兼容旧代码：保留全局函数接口，内部转发至 FontManager 单例
// ============================================================================
// 注意：customFont / fontPath 等全局变量已移除，请使用 FontManager::GetInstance()
inline void LoadCustomFont() { FontManager::GetInstance().LoadCustomFont(); }
inline void EnsureFont(const char* text) { FontManager::GetInstance().EnsureFont(text); }
inline void DrawTextCustom(const char* text, RLVector2 pos, int fontSize, RLColor color, int spacing = 0) {
    FontManager::GetInstance().DrawText(text, pos, fontSize, color, spacing);
}
inline RLVector2 MeasureTextCustom(const char* text, int fontSize, int spacing) {
    return FontManager::GetInstance().MeasureText(text, fontSize, spacing);
}

