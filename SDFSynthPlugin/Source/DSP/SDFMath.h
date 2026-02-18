#pragma once
#include <cmath>
#include <algorithm>

namespace sdf { namespace ops {

inline float smoothMin(float a, float b, float k) {
    if (k < 0.001f) return std::min(a, b);
    float h = std::max(k - std::abs(a - b), 0.f) / k;
    return std::min(a, b) - h * h * h * k / 6.f;
}

}} // namespace sdf::ops
