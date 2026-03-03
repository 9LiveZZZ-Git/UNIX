#pragma once
#include <cmath>
#include <algorithm>

namespace sdf { namespace ops {

inline float smoothMin(float a, float b, float k) {
    if (k < 0.001f) return std::min(a, b);
    float h = std::max(k - std::abs(a - b), 0.f) / k;
    return std::min(a, b) - h * h * h * k / 6.f;
}

inline float smoothIntersection(float d1, float d2, float k) {
    float h = std::clamp(0.5f - 0.5f * (d2 - d1) / k, 0.f, 1.f);
    return d2 + (d1 - d2) * h + k * h * (1.f - h);
}

inline float smoothSubtraction(float d1, float d2, float k) {
    float h = std::clamp(0.5f - 0.5f * (d2 + d1) / k, 0.f, 1.f);
    return d2 + (-d1 - d2) * h + k * h * (1.f - h);
}

inline float chamferUnion(float a, float b, float r) {
    return std::min(std::min(a, b), (a - r + b) * 0.7071f);
}

inline float chamferIntersection(float a, float b, float r) {
    return std::max(std::max(a, b), (a + r + b) * 0.7071f);
}

inline float chamferSubtraction(float a, float b, float r) {
    return chamferIntersection(a, -b, r);
}

inline float stairsUnion(float a, float b, float r, float n) {
    float s = r / n;
    float u = b - r;
    float mod_val = u - a + s;
    float two_s = 2.f * s;
    float m = mod_val - two_s * std::floor(mod_val / two_s);
    return std::min(std::min(a, b), 0.5f * (u + a + std::abs(m - s)));
}

inline float pipe(float a, float b, float r) {
    return std::sqrt(a * a + b * b) - r;
}

}} // namespace sdf::ops
