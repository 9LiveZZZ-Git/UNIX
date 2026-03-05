#include <juce_core/juce_core.h>
#include "DSP/SVFFilter.h"
#include <cmath>

class SVFFilterTests : public juce::UnitTest
{
public:
    SVFFilterTests() : juce::UnitTest("SVFFilter") {}

    void runTest() override
    {
        beginTest("Nyquist guard - cutoff clamped below Nyquist");
        {
            SVFFilter f;
            f.reset();
            // Setting cutoff above Nyquist should not blow up
            f.setParams(30000.f, 0.5f, 44100.f);
            float out = 0.f;
            for (int i = 0; i < 100; ++i)
                out = f.process(i == 0 ? 1.f : 0.f);
            expect(!std::isnan(out), "Output should not be NaN with cutoff above Nyquist");
            expect(!std::isinf(out), "Output should not be Inf with cutoff above Nyquist");
        }

        beginTest("DC passes through LowPass");
        {
            SVFFilter f;
            f.reset();
            f.setParams(1000.f, 0.f, 44100.f);
            f.setMode(FilterMode::LowPass);
            // Feed DC (1.0) for many samples
            float out = 0.f;
            for (int i = 0; i < 4096; ++i)
                out = f.process(1.f);
            expectWithinAbsoluteError(out, 1.f, 0.01f, "LP should pass DC");
        }

        beginTest("DC rejected by HighPass");
        {
            SVFFilter f;
            f.reset();
            f.setParams(1000.f, 0.f, 44100.f);
            f.setMode(FilterMode::HighPass);
            float out = 0.f;
            for (int i = 0; i < 4096; ++i)
                out = f.process(1.f);
            expectWithinAbsoluteError(out, 0.f, 0.01f, "HP should reject DC");
        }

        beginTest("Mode crossfade produces continuous output");
        {
            SVFFilter f;
            f.reset();
            f.setParams(2000.f, 0.3f, 44100.f);
            f.setMode(FilterMode::LowPass);
            // Warm up
            for (int i = 0; i < 512; ++i)
                f.process(std::sin(2.f * 3.14159f * 440.f * i / 44100.f));
            // Switch mode — should not produce discontinuity
            f.setMode(FilterMode::BandPass);
            float prev = f.process(std::sin(2.f * 3.14159f * 440.f * 512 / 44100.f));
            float maxJump = 0.f;
            for (int i = 513; i < 1024; ++i)
            {
                float s = f.process(std::sin(2.f * 3.14159f * 440.f * i / 44100.f));
                maxJump = std::max(maxJump, std::abs(s - prev));
                prev = s;
            }
            expect(maxJump < 0.5f, "Mode crossfade should prevent large discontinuities");
        }

        beginTest("No NaN with zero input");
        {
            SVFFilter f;
            f.reset();
            f.setParams(1000.f, 0.9f, 44100.f);
            for (int i = 0; i < 1000; ++i)
            {
                float out = f.process(0.f);
                expect(!std::isnan(out), "Should not produce NaN");
            }
        }
    }
};

static SVFFilterTests svfFilterTests;
