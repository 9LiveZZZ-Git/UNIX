#pragma once
#include <cmath>
#include <algorithm>

namespace sdf {

inline float sphere(float x, float y, float z, float r) {
    return std::sqrt(x*x + y*y + z*z) - r;
}

inline float box(float x, float y, float z, float b) {
    float dx = std::abs(x) - b;
    float dy = std::abs(y) - b;
    float dz = std::abs(z) - b;
    float outside = std::sqrt(std::max(dx, 0.f) * std::max(dx, 0.f)
                            + std::max(dy, 0.f) * std::max(dy, 0.f)
                            + std::max(dz, 0.f) * std::max(dz, 0.f));
    float inside = std::min(std::max(dx, std::max(dy, dz)), 0.f);
    return outside + inside;
}

inline float torus(float x, float y, float z, float R, float r) {
    float q = std::sqrt(x*x + z*z) - R;
    return std::sqrt(q*q + y*y) - r;
}

inline float cylinder(float x, float y, float z, float r, float h) {
    float d = std::sqrt(x*x + z*z) - r;
    float dy = std::abs(y) - h;
    float outside = std::sqrt(std::max(d, 0.f) * std::max(d, 0.f)
                            + std::max(dy, 0.f) * std::max(dy, 0.f));
    float inside = std::min(std::max(d, dy), 0.f);
    return outside + inside;
}

inline float octahedron(float x, float y, float z, float s) {
    return (std::abs(x) + std::abs(y) + std::abs(z) - s) * 0.57735f;
}

} // namespace sdf
