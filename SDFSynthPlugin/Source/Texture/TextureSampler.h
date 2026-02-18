#pragma once
#include "TextureSystem.h"
#include <cstdint>
#include <cmath>
#include <algorithm>

class TextureSampler
{
public:
    static float sampleBilinear(const uint8_t* data, int w, int h, float u, float v)
    {
        u = u - std::floor(u);
        v = v - std::floor(v);
        float fx = u * (w - 1), fy = v * (h - 1);
        int ix = static_cast<int>(fx), iy = static_cast<int>(fy);
        float dx = fx - ix, dy = fy - iy;
        int ix1 = (ix + 1) % w, iy1 = (iy + 1) % h;

        float c00 = data[(iy  * w + ix)  * 4] / 255.f;
        float c10 = data[(iy  * w + ix1) * 4] / 255.f;
        float c01 = data[(iy1 * w + ix)  * 4] / 255.f;
        float c11 = data[(iy1 * w + ix1) * 4] / 255.f;
        return c00 * (1 - dx) * (1 - dy) + c10 * dx * (1 - dy) + c01 * (1 - dx) * dy + c11 * dx * dy;
    }

    static float sampleTexCPU(const TextureSlot& slot, float x, float y, float z, float scale)
    {
        if (!slot.loaded || slot.pixelData.empty())
            return 0.f;

        float ax = std::abs(x), ay = std::abs(y), az = std::abs(z);
        float sum = ax + ay + az + 0.001f;
        float wx = ax / sum, wy = ay / sum, wz = az / sum;

        float dx = sampleBilinear(slot.pixelData.data(), slot.width, slot.height,
                                   y * scale * 0.5f + 0.5f, z * scale * 0.5f + 0.5f);
        float dy = sampleBilinear(slot.pixelData.data(), slot.width, slot.height,
                                   x * scale * 0.5f + 0.5f, z * scale * 0.5f + 0.5f);
        float dz = sampleBilinear(slot.pixelData.data(), slot.width, slot.height,
                                   x * scale * 0.5f + 0.5f, y * scale * 0.5f + 0.5f);
        return wx * dx + wy * dy + wz * dz;
    }
};
