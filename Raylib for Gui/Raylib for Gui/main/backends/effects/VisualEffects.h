#ifndef VISUAL_EFFECTS_H
#define VISUAL_EFFECTS_H

#include "raylib.h"

// ============================================================================
// VisualEffects - 从 core_fluent_button.c 移植的 Fluent Design 视觉特效工具集
//
// 包含：
//   - 数学工具（平滑插值、缓动函数）
//   - 裁剪栈（模拟嵌套 scissor）
//   - 软阴影（多层半透明圆角叠加）
//   - Reveal 高光（鼠标跟随径向渐变）
//   - Ripple 涟漪（点击波纹动画助手）
// ============================================================================

// --------------------------------------------------------------------------
// 数学工具
// --------------------------------------------------------------------------
float   LerpF(float a, float b, float t);
float   Clamp01(float x);
float   Smoothstep(float t);
float   ApproachExp(float current, float target, float speed, float dt);
float   EaseOutCubic(float t);
float   EaseInQuad(float t);
float   EaseOutQuad(float t);
RLColor LerpColor(RLColor a, RLColor b, float t);

/// 将圆角像素半径转换为 raylib 的 roundness 值（roundness = 2*r / min(w,h)）
float RoundnessForRadius(float w, float h, float radius);

/// 帧率无关的临界阻尼平滑（Unity SmoothDamp 风格）
float SmoothDampF(float current, float target, float* velocity,
                  float smoothTime, float maxSpeed, float dt);

// --------------------------------------------------------------------------
// 裁剪栈
//  raylib 原生 scissor 不支持嵌套，此栈通过求交集模拟嵌套裁剪
// --------------------------------------------------------------------------
void PushScissorI(int x, int y, int w, int h);
void PushScissorRect(RLRectangle r, int inset);
void PopScissor();
void ScissorReset();

// --------------------------------------------------------------------------
// 软阴影
//  用 8 层半透明圆角矩形叠加出柔和阴影，spread 和偏移随强度变化
// --------------------------------------------------------------------------
void DrawSoftShadowRounded(RLRectangle r, float radius, float strength,
                           RLColor shadowColor);

// --------------------------------------------------------------------------
// Reveal 高光（鼠标跟随径向渐变）
//  在按钮区域内绘制一个以鼠标位置为中心的径向渐变，产生光照效果
// --------------------------------------------------------------------------
void DrawRevealGradientClipped(RLRectangle r, RLVector2 mousePos,
                               float intensity);

// --------------------------------------------------------------------------
// Ripple 涟漪
//  绘制一个圆形扩散波纹，半径从 startRad 到 endRad，透明度从 maxAlpha 衰减
//  返回 false 表示动画已结束
// --------------------------------------------------------------------------
bool DrawRippleEffect(RLRectangle bounds, RLVector2 center,
                      float progress,      // 0..1
                      float maxRadius,
                      RLColor color);

#endif // VISUAL_EFFECTS_H
