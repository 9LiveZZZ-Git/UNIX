#include <juce_core/juce_core.h>
#include "DSP/SDFOscillator.h"
#include "DSP/WavetableGenerator.h"
#include "DSP/SDFScene3D.h"
#include "DSP/ContourExtractor.h"
#include "DSP/ScanMode.h"
#include <cmath>
#include <algorithm>

class WavetableGeneratorTests : public juce::UnitTest
{
public:
    WavetableGeneratorTests() : juce::UnitTest("WavetableGenerator") {}

    void runTest() override
    {
        beginTest("Output values are bounded [-1, 1]");
        {
            SDFScene3D scene;
            // Default scene: sphere + torus
            ContourExtractor ext;
            auto contour = ext.extractContour(scene, 0.f);
            auto table = WavetableGenerator::generate(
                scene, contour, 0.55f, 0.f, 1.f, 3.f, nullptr, 0.f, nullptr, 0.f, 1.5f);
            for (int i = 0; i < sdf::TABLE_SIZE; ++i)
            {
                expect(table[i] >= -1.f && table[i] <= 1.f,
                       "Sample " + juce::String(i) + " out of bounds: " + juce::String(table[i]));
            }
        }

        beginTest("Mipmap energy conservation - levels have similar energy");
        {
            // Create a wavetable with harmonics
            WavetableGenerator::Wavetable base{};
            for (int i = 0; i < sdf::TABLE_SIZE; ++i)
            {
                float t = sdf::TWO_PI * i / static_cast<float>(sdf::TABLE_SIZE);
                base[i] = std::sin(t) + 0.5f * std::sin(2.f * t) + 0.25f * std::sin(4.f * t);
            }
            // Normalize
            float mx = 0.01f;
            for (auto s : base) mx = std::max(mx, std::abs(s));
            for (auto& s : base) s /= mx;

            auto mip = WavetableGenerator::generateMipMap(base);

            // Compute energy of level 0
            float e0 = 0.f;
            for (int i = 0; i < sdf::TABLE_SIZE; ++i)
                e0 += mip.tables[0][i] * mip.tables[0][i];

            // Levels 1-3 should have roughly similar energy (within 50%)
            for (int l = 1; l <= 3; ++l)
            {
                float el = 0.f;
                for (int i = 0; i < sdf::TABLE_SIZE; ++i)
                    el += mip.tables[l][i] * mip.tables[l][i];

                float ratio = (e0 > 0.01f) ? el / e0 : 0.f;
                expect(ratio > 0.5f && ratio < 2.0f,
                       "Mip level " + juce::String(l) + " energy ratio: " + juce::String(ratio));
            }
        }

        beginTest("All 6 scan modes produce valid tables");
        {
            SDFScene3D scene;
            ContourExtractor ext;
            auto contour = ext.extractContour(scene, 0.f);

            for (int m = 0; m < 6; ++m)
            {
                auto mode = static_cast<ScanMode>(m);
                auto table = WavetableGenerator::generateWithMode(
                    mode, scene, contour, 0.55f, 0.f, 0.5f, 3.f,
                    nullptr, 0.f, nullptr, 0.f, 1.5f);

                bool allZero = true;
                bool hasNaN = false;
                for (int i = 0; i < sdf::TABLE_SIZE; ++i)
                {
                    if (table[i] != 0.f) allZero = false;
                    if (std::isnan(table[i])) hasNaN = true;
                }
                expect(!hasNaN, "Mode " + juce::String(m) + " produced NaN");
                expect(!allZero, "Mode " + juce::String(m) + " produced all zeros");
            }
        }
    }
};

static WavetableGeneratorTests wavetableGeneratorTests;
