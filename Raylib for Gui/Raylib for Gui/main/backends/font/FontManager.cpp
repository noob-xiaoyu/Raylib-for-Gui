#include "FontManager.h"
#include <string>
#include <unordered_set>
#include <cctype>
#include "../logsys/logs.h"

#ifdef _WIN32
#include <windows.h>
#include <Shlobj.h>
#undef DrawText  // 防止 windows.h 的 DrawText 宏与 FontManager::DrawText 方法名冲突
#endif

// ============================================================================
// 单例
// ============================================================================
FontManager& FontManager::GetInstance() {
    static FontManager instance;
    return instance;
}

// ============================================================================
// 内部工具
// ============================================================================
std::vector<int> FontManager::ExtractCodepoints(const char* text) {
    int count = 0;
    int* cps = RLLoadCodepoints(text, &count);
    std::vector<int> result(cps, cps + count);
    RLUnloadCodepoints(cps);
    return result;
}

// ============================================================================
// 字体重建
// ============================================================================
void FontManager::RebuildFont() {
    if (m_codepoints.empty() || m_fontPath.empty()) return;

    RLFont newFont = RLLoadFontEx(
        m_fontPath.c_str(),
        m_fontSizeGlobal,
        m_codepoints.data(),
        static_cast<int>(m_codepoints.size())
    );

    if (newFont.texture.id == 0 || newFont.baseSize <= 0) return;

    if (m_font.texture.id != 0 && !m_usingDefaultFont) {
        RLUnloadFont(m_font);
    }

    m_font = newFont;
    m_usingDefaultFont = false;
    RLSetTextureFilter(m_font.texture, RL_E_TEXTURE_FILTER_BILINEAR);
}

// ============================================================================
// 初始化加载字体
// ============================================================================
void FontManager::LoadCustomFont() {
    if (m_font.baseSize != 0) return;

    const char* initText = "1234567890";
    auto cps = ExtractCodepoints(initText);
    for (int cp : cps) {
        if (m_codepointSet.insert(cp).second) {
            m_codepoints.push_back(cp);
        }
    }

    // 1) 尝试从 exe 同路径的 font 文件夹加载
    const char* appDir = RLGetApplicationDirectory();
    if (appDir && appDir[0]) {
        std::string fontDir = appDir;
#ifdef _WIN32
        fontDir += "\\font\\";
#else
        fontDir += "/font/";
#endif
        RLFilePathList allFiles = RLLoadDirectoryFilesEx(fontDir.c_str(), "*.*", false);
        for (unsigned int i = 0; i < allFiles.count; ++i) {
            std::string fullPath = allFiles.paths[i];
            size_t dotPos = fullPath.find_last_of('.');
            if (dotPos == std::string::npos) continue;
            std::string ext = fullPath.substr(dotPos + 1);
            for (char& c : ext) c = static_cast<char>(std::tolower(c));
            if (ext != "ttf" && ext != "ttc" && ext != "otf") continue;

            if (!RLFileExists(fullPath.c_str())) continue;

            RLFont tryFont = RLLoadFontEx(fullPath.c_str(), m_fontSizeGlobal,
                                          m_codepoints.data(), static_cast<int>(m_codepoints.size()));
            if (tryFont.texture.id == 0 || tryFont.baseSize <= 0) continue;

            m_font = tryFont;
            m_fontPath = fullPath;
            m_usingDefaultFont = false;
            RLSetTextureFilter(m_font.texture, RL_E_TEXTURE_FILTER_BILINEAR);

            RLUnloadDirectoryFiles(allFiles);
            Log::cout(Font, "已加载字体: %s", fullPath.c_str());
            return;
        }
        RLUnloadDirectoryFiles(allFiles);
    }

    // 2) 回退到系统字体目录
    std::string fontDir;
#ifdef _WIN32
    wchar_t fontPathW[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_FONTS, nullptr, 0, fontPathW))) {
        char narrowPath[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, fontPathW, -1, narrowPath, MAX_PATH, nullptr, nullptr);
        fontDir = narrowPath;
    } else {
        fontDir = "C:\\Windows\\Fonts";
    }
#else
    fontDir = "/usr/share/fonts";
#endif

    const char* fontFiles[] = { "simhei.ttf", "msyh.ttc", "verdana.ttf", nullptr };
    for (int i = 0; fontFiles[i]; ++i) {
        std::string fullPath = fontDir;
#ifdef _WIN32
        fullPath += "\\";
#else
        fullPath += "/";
#endif
        fullPath += fontFiles[i];
        if (!RLFileExists(fullPath.c_str())) continue;

        RLFont tryFont = RLLoadFontEx(fullPath.c_str(), m_fontSizeGlobal,
                                      m_codepoints.data(), static_cast<int>(m_codepoints.size()));
        if (tryFont.texture.id == 0 || tryFont.baseSize <= 0) continue;

        m_font = tryFont;
        m_fontPath = fullPath;
        m_usingDefaultFont = false;
        RLSetTextureFilter(m_font.texture, RL_E_TEXTURE_FILTER_BILINEAR);
        Log::cout(Font, "已加载系统字体: %s", fullPath.c_str());
        return;
    }

    Log::cout(Font, "警告: 未找到可用字体，使用默认字体");
}

// ============================================================================
// EnsureFont - 确保字符已加载
// ============================================================================
void FontManager::EnsureFont(const char* text) {
    auto cps = ExtractCodepoints(text);
    bool needRebuild = false;
    for (int cp : cps) {
        if (m_codepointSet.find(cp) == m_codepointSet.end()) {
            m_codepointSet.insert(cp);
            m_codepoints.push_back(cp);
            needRebuild = true;
        }
    }
    if (needRebuild) {
        RebuildFont();
    }
}

// ============================================================================
// 绘制 & 测量
// ============================================================================
void FontManager::DrawText(const char* text, RLVector2 pos, int fontSize, RLColor color, int spacing) {
    EnsureFont(text);
    if (m_font.texture.id != 0) {
        RLDrawTextEx(m_font, text, pos, static_cast<float>(fontSize), static_cast<float>(spacing), color);
    } else {
        RLDrawText(text, static_cast<int>(pos.x), static_cast<int>(pos.y), fontSize, color);
    }
}

RLVector2 FontManager::MeasureText(const char* text, int fontSize, int spacing) {
    EnsureFont(text);
    if (m_font.texture.id != 0) {
        return RLMeasureTextEx(m_font, text, static_cast<float>(fontSize), static_cast<float>(spacing));
    }
    return RLVector2{
        static_cast<float>(RLMeasureText(text, fontSize)),
        static_cast<float>(fontSize)
    };
}

// ============================================================================
// 释放资源
// ============================================================================
void FontManager::Unload() {
    if (m_font.texture.id != 0 && !m_usingDefaultFont) {
        RLUnloadFont(m_font);
        m_font = { 0 };
    }
    m_fontPath.clear();
    m_codepointSet.clear();
    m_codepoints.clear();
}