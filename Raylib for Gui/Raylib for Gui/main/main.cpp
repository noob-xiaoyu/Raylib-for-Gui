#include "raylib.h"
#include "backends/z-index/z_index.h"
#include "backends/font/FontManager.h"
#include "backends/gui/gui_manager.h"
#include "backends/gui/gui_panel.h"
#include "backends/effects/VisualEffects.h"
#include "backends/logsys/logs.h"
#include <cstdio>
#include <string>
#include <format>
#include "gui/gui_multidropdown.h"
#include "gui/gui_dropdown.h"
#include "gui/gui_button.h"
#include "gui/gui_label.h"
#include "gui/gui_slider.h"
#include "gui/gui_toggle.h"
#include "gui/gui_TextBox.h"

// ============================================================================
// 辅助：创建带圆角面板 + 标题的面板容器
// ============================================================================
struct PanelSection {
    GUIPanel*   bg;
    GUILabel*   title;
    float       x, y, w, h;
};

static PanelSection MakeSection(GUIManager& gui, int& nextId,
                                float x, float y, float w, float h,
                                const char* titleText) {
    int idBase = nextId;
    nextId += 10;

    GUIPanel* bg = gui.createWidget<GUIPanel>(idBase + 0, 2,
        x, y, w, h);
    bg->setColor(RLColor{ 35, 35, 55, 200 });
    bg->setRoundness(0.06f);
    bg->setBorderWidth(1);
    bg->setBorderColor(RLColor{ 60, 60, 85, 255 });

    GUILabel* lbl = gui.createWidget<GUILabel>(idBase + 1, 3,
        titleText, x + 12, y + 8, 16,
        RLColor{ 160, 180, 255, 255 });

    return { bg, lbl, x, y, w, h };
}

// ============================================================================
// 主函数
// ============================================================================
int main() {
    system("chcp 65001 > nul");
    const int screenWidth  = 1200;
    const int screenHeight = 1200;
    RLSetConfigFlags(RL_E_FLAG_WINDOW_RESIZABLE);
    RLInitWindow(screenWidth, screenHeight, "Raylib for Gui");
    RLSetExitKey(RL_E_KEY_NULL);
    RLSetTargetFPS(60);
    FontManager::GetInstance().LoadCustomFont();

    GUIManager& gui = GUIManager::GetInstance();
    int nextId = 1;   // 全局 ID 分配器

    // ==================================================================
    // 0) 背景
    // ==================================================================
    GUIPanel* bg = gui.createWidget<GUIPanel>(nextId++, 0,
        0, 0, (float)screenWidth, (float)screenHeight);
    bg->setColor(RLColor{ 18, 18, 30, 255 });
    bg->setBorderWidth(0);

    // ==================================================================
    // 标题栏
    // ==================================================================
    GUILabel* title = gui.createWidget<GUILabel>(nextId++, 1,
        "Raylib for Gui", 20, 14, 28,
        RLColor{ 220, 220, 255, 255 });

    GUILabel* subtitle = gui.createWidget<GUILabel>(nextId++, 1,
        "Label / Button / Dropdown / MultiDropdown / TextBox / Slider / Toggle",
        20, 48, 14, RLColor{ 120, 120, 160, 255 });

    // ==================================================================
    // 底部状态栏
    // ==================================================================
    GUIPanel* stBar = gui.createWidget<GUIPanel>(nextId++, 5,
        0, screenHeight - 30.0f, (float)screenWidth, 30.0f);
    stBar->setColor(RLColor{ 25, 25, 45, 230 });
    stBar->setBorderWidth(0);

    GUILabel* stFps = gui.createWidget<GUILabel>(nextId++, 6,
        "", 10, (float)(screenHeight - 26), 14,
        RLColor{ 140, 140, 180, 255 });

    // ==================================================================
    // 区域 1 —— 按钮 (左上)
    // ==================================================================
    auto s1 = MakeSection(gui, nextId, 20.0f, 80.0f, 220.0f, 190.0f, "▎按钮 Button");
    {
        GUIButton* btn1 = gui.createWidget<GUIButton>(nextId++, 3, "默认风格",     35, 115, 190, 32);
        GUIButton* btn2 = gui.createWidget<GUIButton>(nextId++, 3, "无圆角",       35, 155, 190, 32);
        btn2->setRoundness(0.0f);
        btn2->setColor(RLColor{ 70, 70, 100, 255 });
        GUIButton* btn3 = gui.createWidget<GUIButton>(nextId++, 3, "扁平风格",     35, 195, 190, 32);
        btn3->setRoundness(0.0f);
        btn3->setNormalColor(RLColor{ 55, 55, 85, 255 });
        btn3->setHoverColor(RLColor{ 75, 75, 110, 255 });

        btn1->setOnClick([]{ Log::cout(Raylib, "[button] 点击了「默认风格」"); });
        btn2->setOnClick([]{ Log::cout(Raylib, "[button] 点击了「无圆角」"); });
        btn3->setOnClick([]{ Log::cout(Raylib, "[button] 点击了「扁平风格」"); });
    }

    // ==================================================================
    // 区域 2 —— 下拉框 (左中)
    // ==================================================================
    auto s2 = MakeSection(gui, nextId, 20.0f, 285.0f, 220.0f, 200.0f, "▎下拉框 Dropdown");
    {
        GUIDropdown* dd = gui.createWidget<GUIDropdown>(nextId++, 3, 35, 320, 190, 32);
        dd->addItem("C++"); dd->addItem("Python"); dd->addItem("JavaScript");
        dd->addItem("Rust"); dd->addItem("Go"); dd->addItem("Java"); dd->addItem("C#");
        dd->setSelectedIndex(0);
        dd->setRoundness(0.0f);
        dd->setOnSelect([](int idx, const std::string& t) {
            Log::cout(Raylib, "[Dropdown] 选择了: %s", t.c_str());
        });

        GUILabel* ddHint = gui.createWidget<GUILabel>(nextId++, 3,
            "单选下拉", 35, 360, 13,
            RLColor{ 130, 130, 170, 255 });

        GUIMultiDropdown* md = gui.createWidget<GUIMultiDropdown>(nextId++, 3, 35, 390, 190, 32);
        md->addItem("C++"); md->addItem("Python"); md->addItem("JavaScript");
        md->addItem("Rust"); md->addItem("Go"); md->addItem("Java"); md->addItem("C#");
        md->setRoundness(0.0f);
        md->setOnSelect([](int idx, const std::string& t, bool sel) {
            Log::cout(Raylib, "[MultiDropdown] %s: %s", sel ? "✓选中" : "✕取消", t.c_str());
        });

        GUILabel* mdHint = gui.createWidget<GUILabel>(nextId++, 3,
            "多选下拉", 35, 430, 13,
            RLColor{ 130, 130, 170, 255 });
    }

    // ==================================================================
    // 区域 3 —— Slider (左下)
    // ==================================================================
    auto s3 = MakeSection(gui, nextId, 20.0f, 500.0f, 220.0f, 230.0f, "▎滑块 Slider");
    {
        auto MakeSlider = [&](float y, const char* label, double val, double max, int idOff) {
            GUISlider* s = gui.createWidget<GUISlider>(
                nextId++, 3, 35, y, 170, 28);
            s->SetRange(0.0, max);
            s->SetValue(val);
            s->SetContinuous(true);
            s->SetLabel(label);
            s->SetShowValue(true);
            s->SetFontSize(14);
            s->SetTrackColor(RLColor{ 50, 50, 75, 255 });
            s->SetFillColor(RLColor{ 100, 150, 255, 255 });
            s->SetThumbColor(RLColor{ 200, 200, 230, 255 });
            s->SetThumbHoverColor(RLColor{ 220, 220, 250, 255 });
            s->SetThumbDragColor(RLColor{ 240, 240, 255, 255 });
            s->SetShowInputBox(true);       // 启用内嵌数值输入框
            s->SetInputBoxWidth(50.0f);     // 输入框宽度 50px
            return s;
        };

        GUISlider* sA = MakeSlider(535, "音量", 75.0, 100.0, 0);
        GUISlider* sB = MakeSlider(570, "亮度", 50.0, 100.0, 1);
        GUISlider* sC = MakeSlider(605, "对比度", 30.0, 200.0, 2);

        sA->SetOnChange([](double v) { Log::cout(Raylib, "[滑块] 音量: %.0f", v); });
        sB->SetOnChange([](double v) { Log::cout(Raylib, "[滑块] 亮度: %.0f", v); });
        sC->SetOnChange([](double v) { Log::cout(Raylib, "[滑块] 对比度: %.0f", v); });

        // 整数滑块演示
        GUISlider* sD = gui.createWidget<GUISlider>(nextId++, 3, 35, 640, 170, 28,
            SliderType::Int);
        sD->SetRange(0.0, 10.0);
        sD->SetValue(5.0);
        sD->SetStep(1.0);
        sD->SetLabel("评分");
        sD->SetShowValue(true);
        sD->SetFontSize(14);
        sD->SetTrackColor(RLColor{ 50, 50, 75, 255 });
        sD->SetFillColor(RLColor{ 200, 160, 80, 255 });
        sD->SetThumbColor(RLColor{ 200, 200, 230, 255 });
        sD->SetShowInputBox(true);
        sD->SetInputBoxWidth(50.0f);
        sD->SetOnChange([](double v) { Log::cout(Raylib, "[滑块] 评分: %d", (int)v); });
    }

    // ==================================================================
    // 区域 4 —— TextBox 变种展示 (右侧)
    // ==================================================================
    auto s4 = MakeSection(gui, nextId, 260.0f, 80.0f, 920.0f, 610.0f, "▎文本框 TextBox 变种");
    {
        // --- 4a. 只读日志框 (左上) ---
        GUILabel* lbla = gui.createWidget<GUILabel>(nextId++, 3,
            "📋 只读日志 (ReadOnly + CodeEditor)", 275, 110, 14,
            RLColor{ 180, 200, 255, 255 });

        TextBox* logBox = gui.createWidget<TextBox>(nextId++, 3,
            275, 128, 430, 150);
        logBox->SetFont(15.0f, 1.0f, 20.0f);
        logBox->SetStyle(TextBoxStyle::ReadOnly);
        logBox->SetShowLineNumbers(true);
        logBox->SetText(
            "[INFO]  系统初始化完成\n"
            "[INFO]  已加载 6 个控件类型\n"
            "[WARN]  配置文件中未找到主题设置\n"
            "[INFO]  使用默认深色主题\n"
            "[INFO]  字体渲染器已就绪\n"
            "[OK]    RL_Gui v1.0 启动成功\n");

        // --- 4b. 单行/密码框 (右上) ---
        GUILabel* lblb = gui.createWidget<GUILabel>(nextId++, 3,
            "🔑 密码输入 (SingleLine + Password)", 730, 110, 14,
            RLColor{ 255, 220, 180, 255 });

        TextBox* pwBox = gui.createWidget<TextBox>(nextId++, 3,
            730, 128, 430, 40);
        pwBox->SetFont(18.0f, 1.0f, 24.0f);
        pwBox->SetStyle(TextBoxStyle::SingleLine);
        pwBox->SetPasswordMode(true);
        pwBox->SetPlaceholder("请输入密码...");
        pwBox->SetText("MyPassword123");

        // --- 4c. 数值输入 (第二行左) ---
        GUILabel* lblc = gui.createWidget<GUILabel>(nextId++, 3,
            "🔢 数值输入 (Numeric)", 275, 290, 14,
            RLColor{ 180, 255, 200, 255 });

        TextBox* numBox = gui.createWidget<TextBox>(nextId++, 3,
            275, 308, 200, 40);
        numBox->SetFont(20.0f, 1.0f, 26.0f);
        numBox->SetStyle(TextBoxStyle::Numeric);
        numBox->SetNumericRange(0.0, 100.0);
        numBox->SetNumericValue(42.5);

        // --- 4d. PopupInput (第二行右) ---
        GUILabel* lbld = gui.createWidget<GUILabel>(nextId++, 3,
            "💬 Popup 输入 (点我弹出编辑器)", 730, 290, 14,
            RLColor{ 200, 200, 255, 255 });

        TextBox* popupTrigger = gui.createWidget<TextBox>(nextId++, 3,
            730, 308, 200, 40);
        popupTrigger->SetFont(16.0f, 1.0f, 22.0f);
        popupTrigger->SetStyle(TextBoxStyle::PopupInput);
        popupTrigger->SetPopupLabel("编辑内容：");
        popupTrigger->SetPlaceholder("点击弹出编辑窗口...");
        popupTrigger->SetPopupCallback([](double val) {
            Log::cout(Raylib, "[Popup] 用户提交数值: %.2f", val);
        });

        // --- 4e. 开关 Toggle (面板中部空位) ---
        GUILabel* lblToggle = gui.createWidget<GUILabel>(nextId++, 3,
            "🔘 开关 Toggle", 500, 290, 14,
            RLColor{ 255, 230, 180, 255 });

        GUIToggle* tg1 = gui.createWidget<GUIToggle>(nextId++, 3,
            "", 500, 312, true);
        tg1->setOnToggle([](bool v) {
            Log::cout(Raylib, "[Toggle] 音效: %s", v ? "✓ 开启" : "✕ 关闭");
        });

        GUIToggle* tg2 = gui.createWidget<GUIToggle>(nextId++, 3,
            "自动更新", 500, 344, false);
        tg2->setOnToggle([](bool v) {
            Log::cout(Raylib, "[Toggle] 自动更新: %s", v ? "✓ 开启" : "✕ 关闭");
        });
        tg2->setTrackOnColor(RLColor{ 80, 160, 220, 255 });

        // --- 4f. 多行文本编辑器 (底部大面积) ---
        GUILabel* ble = gui.createWidget<GUILabel>(nextId++, 3,
            "📝 多行文本编辑器", 275, 368, 14,
            RLColor{ 200, 180, 255, 255 });

        TextBox* editor = gui.createWidget<TextBox>(nextId++, 3,
            275, 386, 885, 285);
        editor->SetFont(16.0f, 1.0f, 22.0f);
        //editor->SetStyle(TextBoxStyle::CodeEditor);
        editor->SetText(
            "// RL_Gui 文本编辑器演示\n"
            "#include <iostream>\n"
            "\n"
            "int main() {\n"
            "    // 这是一个多行文本编辑器\n"
            "    std::string msg = \"Hello, RL_Gui!\";\n"
            "    std::cout << msg << std::endl;\n"
            "\n"
            "    for (int i = 0; i < 10; i++) {\n"
            "        std::cout << \"计数: \" << i << std::endl;\n"
            "    }\n"
            "\n"
            "    return 0;\n"
            "}\n"
            "\n"
            "快捷键: Ctrl+C/V/X 复制粘贴剪切\n"
            "Ctrl+Z/Y 撤销重做 | Ctrl+A 全选\n");
    }

    // ==================================================================
    // 区域 5 —— Fluent Design 视觉特效演示 (右下)
    // ==================================================================
    // 全局动效状态（在 section 内外共享）
    bool* pFxToggle = new bool(true);
    GUILabel* lblFxStatus = nullptr;

    auto s5 = MakeSection(gui, nextId, 260.0f, 700.0f, 920.0f, 450.0f, "▎✨ Fluent Design 视觉特效 (移植自 raylib官方 core_fluent_button.c)");
    {
        // 说明标签
        gui.createWidget<GUILabel>(nextId++, 3,
            "按钮悬停时显示 软阴影 + Reveal 高光 + 微缩放，按下触发 Ripple 涟漪，Tab 聚焦显示 Focus Ring",
            275, 730, 14, RLColor{ 150, 200, 255, 255 });

        // --- 5a. 演示按钮组：动效开/关控制 ---
        GUIButton* btnFxOn  = gui.createWidget<GUIButton>(nextId++, 3,
            "✅ 动效开 (Fluent)", 275, 760, 200, 42);
        btnFxOn->setRoundness(0.3f);
        btnFxOn->setNormalColor(RLColor{ 40, 80, 140, 255 });
        btnFxOn->setHoverColor(RLColor{ 60, 120, 200, 255 });
        btnFxOn->setPressColor(RLColor{ 25, 50, 90, 255 });

        GUIButton* btnFxOff = gui.createWidget<GUIButton>(nextId++, 3,
            "❌ 动效关 (Classic)", 490, 760, 200, 42);
        btnFxOff->setRoundness(0.3f);
        btnFxOff->setNormalColor(RLColor{ 80, 50, 50, 255 });
        btnFxOff->setHoverColor(RLColor{ 130, 70, 70, 255 });
        btnFxOff->setPressColor(RLColor{ 50, 25, 25, 255 });

        // 动效状态指示
        lblFxStatus = gui.createWidget<GUILabel>(nextId++, 3,
            "当前：Fluent 动效已启用  [按 F 切换]",
            275, 810, 15, RLColor{ 200, 230, 255, 255 });

        btnFxOn->setOnClick([pFxToggle, lblFxStatus]() {
            *pFxToggle = true;
            lblFxStatus->setText("当前：Fluent 动效已启用  [按 F 切换]");
            Log::cout(Raylib, "[特效] Fluent 动效已启用");
        });
        btnFxOff->setOnClick([pFxToggle, lblFxStatus]() {
            *pFxToggle = false;
            lblFxStatus->setText("当前：经典模式 (动效关闭)  [按 F 切换]");
            Log::cout(Raylib, "[特效] Fluent 动效已关闭，使用经典模式");
        });

        // --- 5b. 带阴影的面板演示 ---
        GUILabel* lblShadow = gui.createWidget<GUILabel>(nextId++, 3,
            "▸ 面板软阴影 (Shadow)", 275, 840, 15, RLColor{ 180, 220, 255, 255 });

        GUIPanel* shadowPanel = gui.createWidget<GUIPanel>(nextId++, 3,
            275, 865, 190, 110);
        shadowPanel->setRoundness(0.04f);
        shadowPanel->setColor(RLColor{ 40, 40, 65, 255 });
        shadowPanel->setBorderColor(RLColor{ 70, 70, 100, 180 });
        shadowPanel->setBorderWidth(1);
        shadowPanel->setShadowStrength(0.7f);
        shadowPanel->setShadowRadius(14.0f);

        GUILabel* shadowLabel = gui.createWidget<GUILabel>(nextId++, 4,
            "shadowStrength = 0.7", 285, 875, 13, RLColor{ 160, 200, 255, 255 });

        GUIButton* btnShadowUp = gui.createWidget<GUIButton>(nextId++, 4,
            "⬆ 增强", 285, 900, 80, 28);
        btnShadowUp->setRoundness(0.3f);
        btnShadowUp->setFontSize(15);

        GUIButton* btnShadowDn = gui.createWidget<GUIButton>(nextId++, 4,
            "⬇ 减弱", 375, 900, 80, 28);
        btnShadowDn->setRoundness(0.3f);
        btnShadowDn->setFontSize(15);

        btnShadowUp->setOnClick([shadowPanel, shadowLabel]() {
            float s = std::min(1.0f, shadowPanel->getShadowStrength() + 0.1f);
            shadowPanel->setShadowStrength(s);
            shadowLabel->setText(
                std::format("shadowStrength = {:.1f}", s).c_str());
            Log::cout(Raylib, "[阴影] 强度: %.1f", s);
        });
        btnShadowDn->setOnClick([shadowPanel, shadowLabel]() {
            float s = std::max(0.0f, shadowPanel->getShadowStrength() - 0.1f);
            shadowPanel->setShadowStrength(s);
            shadowLabel->setText(
                std::format("shadowStrength = {:.1f}", s).c_str());
            Log::cout(Raylib, "[阴影] 强度: %.1f", s);
        });

        // --- 5c. 无阴影对照面板 ---
        GUILabel* lblNoShadow = gui.createWidget<GUILabel>(nextId++, 3,
            "▸ 无阴影 (对照)", 500, 840, 15, RLColor{ 180, 220, 255, 255 });

        GUIPanel* noShadowPanel = gui.createWidget<GUIPanel>(nextId++, 3,
            500, 865, 190, 110);
        noShadowPanel->setRoundness(0.04f);
        noShadowPanel->setColor(RLColor{ 40, 40, 65, 255 });
        noShadowPanel->setBorderColor(RLColor{ 70, 70, 100, 180 });
        noShadowPanel->setBorderWidth(1);
        noShadowPanel->setShadowStrength(0.0f);

        GUILabel* noShadowLabel = gui.createWidget<GUILabel>(nextId++, 4,
            "shadowStrength = 0.0 (无阴影)", 510, 875, 13, RLColor{ 140, 140, 180, 255 });

        // --- 5d. Reveal + Ripple 演示按钮 ---
        GUILabel* lblReveal = gui.createWidget<GUILabel>(nextId++, 3,
            "▸ Reveal 高光 + Ripple 涟漪演示", 720, 840, 15, RLColor{ 180, 220, 255, 255 });

        GUIButton* btnRevealDemo = gui.createWidget<GUIButton>(nextId++, 3,
            "✨ 悬停看 Reveal 高光", 720, 865, 250, 48);
        btnRevealDemo->setRoundness(0.4f);
        btnRevealDemo->setNormalColor(RLColor{ 60, 50, 90, 255 });
        btnRevealDemo->setHoverColor(RLColor{ 90, 70, 140, 255 });
        btnRevealDemo->setPressColor(RLColor{ 35, 25, 55, 255 });
        btnRevealDemo->setOnClick([pFxToggle]() {
            Log::cout(Raylib, "[特效] Reveal+Ripple 按钮被点击！");
        });

        GUIButton* btnRippleClick = gui.createWidget<GUIButton>(nextId++, 3,
            "🌊 点我触发 Ripple", 720, 925, 250, 48);
        btnRippleClick->setRoundness(0.4f);
        btnRippleClick->setNormalColor(RLColor{ 40, 70, 80, 255 });
        btnRippleClick->setHoverColor(RLColor{ 60, 100, 120, 255 });
        btnRippleClick->setPressColor(RLColor{ 20, 40, 50, 255 });
        btnRippleClick->setOnClick([pFxToggle]() {
            Log::cout(Raylib, "[特效] 点击 Ripple 演示按钮！");
        });

        // --- 5e. Focus Ring 演示说明 ---
        GUILabel* lblFocusRing = gui.createWidget<GUILabel>(nextId++, 3,
            "Tab 键切换焦点 → 观察 Focus Ring (脉冲发光边框)",
            720, 985, 14, RLColor{ 140, 180, 220, 255 });
    }

    // ==================================================================
    // 热更新布局锚点设置
    // 为每个控件设置锚点，使其在窗口缩放时自动调整位置/大小
    // ==================================================================
    {
        const float baseW = (float)screenWidth;   // 1200
        const float baseH = (float)screenHeight;  // 1200

        // 背景面板：完全拉伸
        bg->setAnchors(Layout_StretchAll, baseW, baseH);

        // 标题栏：左上角固定，保持原始文字大小
        title->setAnchors(Layout_TopLeft, baseW, baseH);
        title->setKeepSize(true);
        subtitle->setAnchors(Layout_TopLeft, baseW, baseH);
        subtitle->setKeepSize(true);

        // 底部状态栏：底部拉伸
        stBar->setAnchors(Layout_StretchH | Layout_Bottom, baseW, baseH);
        stFps->setAnchors(Layout_BottomLeft, baseW, baseH);
        stFps->setKeepSize(true);

        // 区域 1~3（左侧三块）：左上锚定，等比缩放
        s1.bg->setAnchors(Layout_TopLeft, baseW, baseH);
        s1.title->setAnchors(Layout_TopLeft, baseW, baseH);
        s1.title->setKeepSize(true);

        s2.bg->setAnchors(Layout_TopLeft, baseW, baseH);
        s2.title->setAnchors(Layout_TopLeft, baseW, baseH);
        s2.title->setKeepSize(true);

        s3.bg->setAnchors(Layout_TopLeft, baseW, baseH);
        s3.title->setAnchors(Layout_TopLeft, baseW, baseH);
        s3.title->setKeepSize(true);

        // 区域 4（文本框区域，右半侧）：拉伸
        s4.bg->setAnchors(Layout_StretchAll, baseW, baseH);
        s4.title->setAnchors(Layout_TopLeft, baseW, baseH);
        s4.title->setKeepSize(true);

        // 区域 5（视觉特效区域，右下）：水平拉伸 + 底部锚定
        s5.bg->setAnchors(Layout_StretchH | Layout_Bottom, baseW, baseH);
        s5.title->setAnchors(Layout_TopLeft, baseW, baseH);
        s5.title->setKeepSize(true);

        // 自动推断所有子控件（位于 panel 内的控件）的锚点
        // 策略：根据 x 坐标判断属于左侧还是右侧区域
        for (int id = 1; id < nextId; id++) {
            GUIWidget* w = gui.getWidget(id);
            if (!w || w == bg || w == title || w == subtitle ||
                w == stBar || w == stFps ||
                w == s1.bg || w == s1.title ||
                w == s2.bg || w == s2.title ||
                w == s3.bg || w == s3.title ||
                w == s4.bg || w == s4.title ||
                w == s5.bg || w == s5.title)
                continue;

            float wx = w->getX();
            // 左侧面板内的子控件：左上锚定
            if (wx < 250.0f) {
                w->setAnchors(Layout_TopLeft, baseW, baseH);
            } else {
                // 右侧区域的控件：左上锚定（父容器的缩放会通过 setAnchors 的比例缩放处理）
                w->setAnchors(Layout_TopLeft, baseW, baseH);
            }

            // 标签控件保持原始大小（文字不应随窗口缩放）
            GUILabel* lbl = dynamic_cast<GUILabel*>(w);
            if (lbl) w->setKeepSize(true);
        }

        // 特殊处理：区域 4 内的 TextBox 编辑器 → 拉伸
        // 找到最后一个创建的 TextBox（大编辑器）
        for (int id = nextId - 1; id >= 1; id--) {
            GUIWidget* w = gui.getWidget(id);
            if (!w) continue;
            TextBox* tb = dynamic_cast<TextBox*>(w);
            if (tb && tb->GetStyle() != TextBoxStyle::PopupInput) {
                // 日志框和编辑器：拉伸
                if (tb->getWidth() > 400.0f && tb->getHeight() > 100.0f) {
                    tb->setAnchors(Layout_StretchAll, baseW, baseH);
                }
            }
        }
    }

    // ==================================================================
    // 主循环
    // ==================================================================
    bool layoutDirty = false;
    while (!RLWindowShouldClose()) {
        // 状态栏
        int fps = RLGetFPS();
        char fpsBuf[64];
        std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d", fps);
        stFps->setText(fpsBuf);

        // F 键切换全局动效
        if (RLIsKeyPressed(RL_E_KEY_F)) {
            *pFxToggle = !(*pFxToggle);
            // 遍历所有按钮切换动效
            for (int id = 1; id < nextId; id++) {
                GUIWidget* w = gui.getWidget(id);
                if (w) {
                    GUIButton* btn = dynamic_cast<GUIButton*>(w);
                    if (btn) btn->setFluentEffects(*pFxToggle);
                }
            }
            lblFxStatus->setText(
                *pFxToggle
                ? "当前：Fluent 动效已启用  [按 F 切换]"
                : "当前：经典模式 (动效关闭)  [按 F 切换]");
            Log::cout(Raylib, "[特效] %s", *pFxToggle ? "启用了 Fluent 动效" : "关闭了 Fluent 动效");
        }

        // ---- 窗口缩放检测 → 热更新布局 ----
        if (RLIsWindowResized()) {
            int newW = RLGetScreenWidth();
            int newH = RLGetScreenHeight();
            if (newW > 0 && newH > 0) {
                // 更新背景和状态栏位置（这些控件使用基类 setPosition/setSize）
                bg->setSize((float)newW, (float)newH);
                stBar->setPosition(0, (float)(newH - 30));
                stBar->setSize((float)newW, 30.0f);
                stFps->setPosition(10, (float)(newH - 26));

                // 对所有设置了锚点的控件应用布局重算
                gui.applyAllLayout((float)newW, (float)newH);

                layoutDirty = false;
                Log::cout(Raylib, "[布局] 窗口缩放至 %d×%d", newW, newH);
            }
        }

        gui.handleEvents();

        RLBeginDrawing();
        RLClearBackground(RLColor{ 15, 15, 25, 255 });
        // 每帧重置 scissor 栈（安全防护）
        ScissorReset();
        gui.render();
        RLEndDrawing();
    }

    delete pFxToggle;
    FontManager::GetInstance().Unload();
    RLCloseWindow();
    return 0;
}