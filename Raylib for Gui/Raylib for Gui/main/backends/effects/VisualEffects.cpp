#include "VisualEffects.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// 数学工具
// ============================================================================

float LerpF(float a, float b, float t) {
    return a + (b - a) * t;
}

float Clamp01(float x) {
    return (x < 0.0f) ? 0.0f : (x > 1.0f ? 1.0f : x);
}

float Smoothstep(float t) {
    t = Clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

float ApproachExp(float current, float target, float speed, float dt) {
    // 帧率无关的指数平滑
    float k = 1.0f - expf(-speed * dt);
    return LerpF(current, target, k);
}

float EaseOutCubic(float t) {
    t = Clamp01(t);
    float u = 1.0f - t;
    return 1.0f - u * u * u;
}

float EaseInQuad(float t) {
    t = Clamp01(t);
    return t * t;
}

float EaseOutQuad(float t) {
    t = Clamp01(t);
    return -t * (t - 2.0f);
}

RLColor LerpColor(RLColor a, RLColor b, float t) {
    t = Clamp01(t);
    RLColor c;
    c.r = (unsigned char)((float)a.r + ((float)b.r - (float)a.r) * t);
    c.g = (unsigned char)((float)a.g + ((float)b.g - (float)a.g) * t);
    c.b = (unsigned char)((float)a.b + ((float)b.b - (float)a.b) * t);
    c.a = (unsigned char)((float)a.a + ((float)b.a - (float)a.a) * t);
    return c;
}

float RoundnessForRadius(float w, float h, float radius) {
    float m = (w < h) ? w : h;
    if (m <= 0.0f) return 0.0f;
    float roundness = (2.0f * radius) / m;
    return Clamp01(roundness);
}

float SmoothDampF(float current, float target, float* velocity,
                  float smoothTime, float maxSpeed, float dt) {
    if (smoothTime < 0.0001f) smoothTime = 0.0001f;
    float omega = 2.0f / smoothTime;

    float x = omega * dt;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    float change = current - target;
    float originalTo = target;

    float maxChange = maxSpeed * smoothTime;
    change = std::clamp(change, -maxChange, maxChange);
    target = current - change;

    float temp = (*velocity + omega * change) * dt;
    *velocity = (*velocity - omega * temp) * exp;

    float output = target + (change + temp) * exp;

    // 防止过冲
    if ((originalTo - current > 0.0f) == (output > originalTo)) {
        output = originalTo;
        *velocity = (output - originalTo) / fmaxf(dt, 0.0001f);
    }

    return output;
}

// ============================================================================
// 裁剪栈
// ============================================================================

#define SCISSOR_STACK_MAX 16

struct ScissorRectI { int x, y, w, h; };

static ScissorRectI gScissorStack[SCISSOR_STACK_MAX];
static int gScissorDepth = 0;

static ScissorRectI ScissorIntersectI(ScissorRectI a, ScissorRectI b) {
    int x1 = (a.x > b.x) ? a.x : b.x;
    int y1 = (a.y > b.y) ? a.y : b.y;
    int x2 = ((a.x + a.w) < (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
    int y2 = ((a.y + a.h) < (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);

    ScissorRectI r;
    r.x = x1;
    r.y = y1;
    r.w = x2 - x1;
    r.h = y2 - y1;

    if (r.w <= 0 || r.h <= 0) {
        // 空交集 → 偏移到屏幕外 1x1，防止多余像素
        int sw = RLGetScreenWidth();
        int sh = RLGetScreenHeight();
        r.x = sw + 16;
        r.y = sh + 16;
        r.w = 1;
        r.h = 1;
    }
    return r;
}

void ScissorReset() {
    if (gScissorDepth > 0) {
        RLEndScissorMode();
        gScissorDepth = 0;
    }
}

void PushScissorI(int x, int y, int w, int h) {
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    ScissorRectI r = { x, y, w, h };
    if (gScissorDepth > 0) {
        r = ScissorIntersectI(gScissorStack[gScissorDepth - 1], r);
    }

    if (gScissorDepth < SCISSOR_STACK_MAX) {
        gScissorStack[gScissorDepth++] = r;
    }
    RLBeginScissorMode(r.x, r.y, r.w, r.h);
}

void PushScissorRect(RLRectangle r, int inset) {
    int x = (int)(r.x + 0.5f) + inset;
    int y = (int)(r.y + 0.5f) + inset;
    int w = (int)(r.width + 0.5f) - inset * 2;
    int h = (int)(r.height + 0.5f) - inset * 2;
    PushScissorI(x, y, w, h);
}

void PopScissor() {
    if (gScissorDepth <= 0) return;
    gScissorDepth--;
    if (gScissorDepth == 0) {
        RLEndScissorMode();
        return;
    }
    ScissorRectI r = gScissorStack[gScissorDepth - 1];
    RLBeginScissorMode(r.x, r.y, r.w, r.h);
}

// ============================================================================
// 软阴影
// ============================================================================

void DrawSoftShadowRounded(RLRectangle r, float radius, float strength,
                           RLColor shadowColor) {
    strength = Clamp01(strength);
    if (strength <= 0.001f) return;

    float round = RoundnessForRadius(r.width, r.height, radius);
    const int seg = 12;
    const int layers = 8;

    float spread = 10.0f + 18.0f * strength;
    float yoff   = 1.5f + 3.0f * strength;
    float baseA  = 26.0f + 22.0f * strength;

    for (int i = 0; i < layers; i++) {
        float t = (float)(i + 1) / (float)layers;
        float expand = spread * t;

        RLRectangle sr = r;
        sr.x -= expand * 0.5f;
        sr.width  += expand;
        sr.y      += yoff * t - expand * 0.18f;
        sr.height += expand;

        float w = (1.0f - t);
        float a = baseA * w * w;
        RLColor c = shadowColor;
        c.a = (unsigned char)(std::clamp(a, 0.0f, 255.0f));
        RLDrawRectangleRounded(sr, round, seg, c);
    }
}

// ============================================================================
// Reveal 高光
// ============================================================================

void DrawRevealGradientClipped(RLRectangle r, RLVector2 mousePos,
                               float intensity) {
    if (intensity <= 0.001f) return;

    // alpha = 35~60 * intensity
    float alpha = LerpF(35.0f, 60.0f, Clamp01(0.5f)) * intensity;

    PushScissorRect(r, 0);
    {
        float rad = 140.0f;
        RLColor inner = RLColor{ 255, 255, 255, 0 };
        inner.a = (unsigned char)(std::clamp(alpha, 0.0f, 255.0f));
        RLColor outerColor = RLColor{ inner.r, inner.g, inner.b, 0 };
        RLDrawCircleGradient((int)mousePos.x, (int)mousePos.y,
                             rad, inner, outerColor);
    }
    PopScissor();
}

// ============================================================================
// Ripple 涟漪
// ============================================================================

bool DrawRippleEffect(RLRectangle bounds, RLVector2 center,
                      float progress,
                      float maxRadius,
                      RLColor color) {
    if (progress <= 0.0f) return true;  // 尚未激活
    float t = Clamp01(progress);
    float e = EaseOutCubic(t);

    float rad = 10.0f + maxRadius * e;

    // 透明度从 55 衰减到 0
    unsigned char a = (unsigned char)(55.0f * (1.0f - t));
    RLColor c = color;
    c.a = (unsigned char)((float)c.a * (1.0f - t));

    PushScissorRect(bounds, 0);
    RLDrawCircleV(center, rad, c);
    PopScissor();

    return (t >= 1.0f); // true = 动画结束
}
