#include <juce_core/juce_core.h>
#include "DSP/VoxelSDF.h"
#include <cmath>

class TestVoxelSDF : public juce::UnitTest
{
public:
    TestVoxelSDF() : UnitTest("VoxelSDF") {}

    void runTest() override
    {
        beginTest("Empty grid returns sentinel");
        {
            VoxelSDF v;
            expectEquals(v.sample(0.f, 0.f, 0.f), 1.5f);
        }

        beginTest("OOB returns sentinel");
        {
            VoxelSDF v;
            v.res = 4;
            v.grid.resize(64, 0.f);
            expectEquals(v.sample(2.f, 0.f, 0.f), 1.5f);
            expectEquals(v.sample(0.f, -2.f, 0.f), 1.5f);
            expectEquals(v.sample(0.f, 0.f, 1.5f), 1.5f);
        }

        beginTest("Uniform grid returns constant");
        {
            VoxelSDF v;
            v.res = 4;
            v.grid.resize(64, 0.42f);
            float s = v.sample(0.f, 0.f, 0.f);
            expectWithinAbsoluteError(s, 0.42f, 1e-5f);
        }

        beginTest("Trilinear interpolation");
        {
            VoxelSDF v;
            v.res = 2;
            v.grid.resize(8, 0.f);
            // grid[0,0,0]=0, grid[1,0,0]=1, rest=0
            v.grid[1] = 1.0f;
            // At x=0 (left) → 0, at x=1 (right) → sample at (1,0,0) should be close to 1
            // Sample at midpoint x=0 → mapped index 0.5
            float mid = v.sample(0.f, -1.f, -1.f);
            expectWithinAbsoluteError(mid, 0.5f, 1e-5f);
        }

        beginTest("Synthetic sphere field");
        {
            VoxelSDF v;
            v.res = 16;
            v.grid.resize(16 * 16 * 16);
            for (int iz = 0; iz < 16; ++iz)
            {
                float z = -1.f + 2.f * static_cast<float>(iz) / 15.f;
                for (int iy = 0; iy < 16; ++iy)
                {
                    float y = -1.f + 2.f * static_cast<float>(iy) / 15.f;
                    for (int ix = 0; ix < 16; ++ix)
                    {
                        float x = -1.f + 2.f * static_cast<float>(ix) / 15.f;
                        float dist = std::sqrt(x * x + y * y + z * z) - 0.5f;
                        v.grid[static_cast<size_t>(ix + iy * 16 + iz * 16 * 16)] = dist;
                    }
                }
            }

            // Center should be inside (negative)
            expect(v.sample(0.f, 0.f, 0.f) < 0.f);
            // Surface should be near zero
            expectWithinAbsoluteError(v.sample(0.5f, 0.f, 0.f), 0.f, 0.15f);
            // Outside should be positive
            expect(v.sample(0.9f, 0.f, 0.f) > 0.f);
        }

        beginTest("data() and byteSize()");
        {
            VoxelSDF v;
            v.res = 4;
            v.grid.resize(64, 1.0f);
            expect(v.data() != nullptr);
            expectEquals(v.byteSize(), static_cast<size_t>(64 * sizeof(float)));
        }
    }
};

static TestVoxelSDF testVoxelSDF;
