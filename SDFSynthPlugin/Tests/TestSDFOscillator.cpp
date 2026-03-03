#include <juce_core/juce_core.h>
#include "DSP/SDFOscillator.h"
#include <cmath>

class SDFOscillatorTests : public juce::UnitTest
{
public:
    SDFOscillatorTests() : juce::UnitTest("SDFOscillator") {}

    void runTest() override
    {
        beginTest("Phase wraps correctly");
        {
            // Create a simple sine wavetable
            MipMappedWavetable mip{};
            for (int i = 0; i < sdf::TABLE_SIZE; ++i)
                mip.tables[0][i] = std::sin(sdf::TWO_PI * i / static_cast<float>(sdf::TABLE_SIZE));
            // Fill mip levels trivially
            for (int l = 1; l < sdf::MIP_LEVELS; ++l)
                mip.tables[l] = mip.tables[0];

            SDFOscillator osc;
            osc.setFrequency(440.f, 44100.f);
            osc.setMipMappedWavetable(&mip);

            // Run for more than one period
            int samplesPerPeriod = static_cast<int>(44100.f / 440.f);
            for (int i = 0; i < samplesPerPeriod * 3; ++i)
                osc.nextSample();

            float phase = osc.getPhase();
            expect(phase >= 0.f && phase < 1.f, "Phase should be in [0, 1)");
        }

        beginTest("Crossfade bounds are correct after off-by-one fix");
        {
            MipMappedWavetable mipA{}, mipB{};
            for (int i = 0; i < sdf::TABLE_SIZE; ++i)
            {
                mipA.tables[0][i] = 0.5f;
                mipB.tables[0][i] = -0.5f;
            }
            for (int l = 1; l < sdf::MIP_LEVELS; ++l)
            {
                mipA.tables[l] = mipA.tables[0];
                mipB.tables[l] = mipB.tables[0];
            }

            SDFOscillator osc;
            osc.setFrequency(440.f, 44100.f);
            osc.setMipMappedWavetable(&mipA);

            // Warm up
            for (int i = 0; i < 10; ++i)
                osc.nextSample();

            // Crossfade to B (pass old table pointer explicitly)
            osc.crossfadeToMipTable(&mipB, &mipA, 64);

            // First sample after crossfade should be close to old value (mostly A)
            float first = osc.nextSample();
            expect(first > -0.6f && first < 0.6f, "First crossfade sample should be bounded");

            // After 64 samples, should be purely B
            for (int i = 1; i < 64; ++i)
                osc.nextSample();

            float last = osc.nextSample();
            expectWithinAbsoluteError(last, -0.5f, 0.01f, "After crossfade, should be entirely B");
        }
    }
};

static SDFOscillatorTests sdfOscillatorTests;
