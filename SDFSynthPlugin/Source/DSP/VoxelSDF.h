#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

struct VoxelSDF
{
    int res = 0;
    std::vector<float> grid;

    float sample(float x, float y, float z) const
    {
        if (res <= 0 || grid.empty())
            return 1.5f;

        // Map [-1,1] → [0, res-1]
        float u = (x * 0.5f + 0.5f) * static_cast<float>(res - 1);
        float v = (y * 0.5f + 0.5f) * static_cast<float>(res - 1);
        float w = (z * 0.5f + 0.5f) * static_cast<float>(res - 1);

        // Out-of-bounds check
        if (u < 0.f || u > static_cast<float>(res - 1) ||
            v < 0.f || v > static_cast<float>(res - 1) ||
            w < 0.f || w > static_cast<float>(res - 1))
            return 1.5f;

        // Trilinear interpolation
        int x0 = std::min(static_cast<int>(u), res - 2);
        int y0 = std::min(static_cast<int>(v), res - 2);
        int z0 = std::min(static_cast<int>(w), res - 2);
        int x1 = x0 + 1;
        int y1 = y0 + 1;
        int z1 = z0 + 1;

        float fx = u - static_cast<float>(x0);
        float fy = v - static_cast<float>(y0);
        float fz = w - static_cast<float>(z0);

        auto idx = [&](int ix, int iy, int iz) -> int {
            return ix + iy * res + iz * res * res;
        };

        float c000 = grid[static_cast<size_t>(idx(x0, y0, z0))];
        float c100 = grid[static_cast<size_t>(idx(x1, y0, z0))];
        float c010 = grid[static_cast<size_t>(idx(x0, y1, z0))];
        float c110 = grid[static_cast<size_t>(idx(x1, y1, z0))];
        float c001 = grid[static_cast<size_t>(idx(x0, y0, z1))];
        float c101 = grid[static_cast<size_t>(idx(x1, y0, z1))];
        float c011 = grid[static_cast<size_t>(idx(x0, y1, z1))];
        float c111 = grid[static_cast<size_t>(idx(x1, y1, z1))];

        float c00 = c000 * (1.f - fx) + c100 * fx;
        float c10 = c010 * (1.f - fx) + c110 * fx;
        float c01 = c001 * (1.f - fx) + c101 * fx;
        float c11 = c011 * (1.f - fx) + c111 * fx;

        float c0 = c00 * (1.f - fy) + c10 * fy;
        float c1 = c01 * (1.f - fy) + c11 * fy;

        return c0 * (1.f - fz) + c1 * fz;
    }

    const float* data() const { return grid.data(); }
    size_t byteSize() const { return grid.size() * sizeof(float); }
};
