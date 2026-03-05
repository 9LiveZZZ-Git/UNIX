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

inline float capsule(float x, float y, float z, float h, float r) {
    float py = y - std::clamp(y, 0.f, h);
    return std::sqrt(x*x + py*py + z*z) - r;
}

inline float roundBox(float x, float y, float z, float bx, float by, float bz, float r) {
    float dx = std::abs(x) - bx;
    float dy = std::abs(y) - by;
    float dz = std::abs(z) - bz;
    float outside = std::sqrt(std::max(dx, 0.f) * std::max(dx, 0.f)
                            + std::max(dy, 0.f) * std::max(dy, 0.f)
                            + std::max(dz, 0.f) * std::max(dz, 0.f));
    float inside = std::min(std::max(dx, std::max(dy, dz)), 0.f);
    return outside + inside - r;
}

inline float hexPrism(float x, float y, float z, float h, float d) {
    // h = circumradius, d = half-depth
    const float k1 = 0.8660254f; // sqrt(3)/2
    float ax = std::abs(x), az = std::abs(z);
    // Fold into one sextant
    float dot = std::min(0.f, -k1 * ax + 0.5f * az);
    ax += 2.f * (-k1) * dot;
    az += 2.f * 0.5f * dot;
    az -= std::clamp(az, 0.f, h);
    float dy = std::abs(y) - d;
    float e = std::sqrt(ax * ax + az * az);
    return std::max(e * ((ax > 0.f) ? 1.f : -1.f), dy);
}

inline float torus82(float x, float y, float z, float R, float r) {
    // L8 outer / L2 inner torus variant
    float px = x, pz = z;
    float lenXZ = std::sqrt(px*px + pz*pz) - R;
    return std::sqrt(lenXZ*lenXZ + y*y) - r;
}

inline float torus88(float x, float y, float z, float R, float r) {
    // L8/L8 norm variant — uses max norms
    float ax = std::abs(x), az = std::abs(z);
    float lenXZ = std::max(ax, az);
    float q = lenXZ - R;
    return std::max(std::abs(q), std::abs(y)) - r;
}

inline float superFormula(float x, float y, float z, float m, float n1, float n2, float n3, float scale) {
    float r = std::sqrt(x*x + y*y + z*z);
    if (r < 1e-8f) return -scale;
    float theta = std::atan2(std::sqrt(x*x + z*z), y);
    float phi = std::atan2(z, x);

    auto sf = [](float angle, float m_, float n1_, float n2_, float n3_) -> float {
        float a = std::abs(std::cos(m_ * angle / 4.f));
        float b = std::abs(std::sin(m_ * angle / 4.f));
        float val = std::pow(a, n2_) + std::pow(b, n3_);
        return (val > 1e-10f) ? std::pow(val, -1.f / n1_) : 1e5f;
    };

    float r1 = sf(theta, m, n1, n2, n3);
    float r2 = sf(phi, m, n1, n2, n3);
    float surfaceR = r1 * r2 * scale;
    return r - surfaceR;
}

} // namespace sdf
