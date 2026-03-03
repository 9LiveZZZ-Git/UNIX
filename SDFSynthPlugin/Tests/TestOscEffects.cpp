#include <juce_core/juce_core.h>
#include "DSP/OscEffects.h"
#include <cmath>

class OscEffectsTest : public juce::UnitTest
{
public:
    OscEffectsTest() : juce::UnitTest("OscEffects") {}

    void runTest() override
    {
        beginTest("All-zero params = passthrough");
        {
            OscEffects fx;
            fx.setParams(0.f, 0.f, 0.5f, 0.f);
            fx.reset();

            float input = 0.75f;
            float output = fx.process(input, 0.3f, 0.001f);
            expectWithinAbsoluteError(output, input, 0.001f);
        }

        beginTest("Phase distortion offset = 0 when pdAmt = 0");
        {
            OscEffects fx;
            fx.setParams(0.f, 0.f, 0.5f, 0.f);
            float offset = fx.getPhaseOffset(0.5f);
            expectWithinAbsoluteError(offset, 0.f, 0.0001f);
        }

        beginTest("PW at center = no effect");
        {
            OscEffects fx;
            fx.setParams(0.f, 0.f, 0.5f, 0.f);
            fx.reset();

            float input = 0.42f;
            float output = fx.process(input, 0.3f, 0.001f);
            expectWithinAbsoluteError(output, input, 0.001f);
        }

        beginTest("Wavefolding ADAA produces no NaN");
        {
            OscEffects fx;
            fx.setParams(1.0f, 0.f, 0.5f, 0.f);
            fx.reset();

            bool valid = true;
            for (int i = 0; i < 10000; ++i)
            {
                float phase = static_cast<float>(i) / 10000.f;
                float input = std::sin(sdf::TWO_PI * phase);
                float output = fx.process(input, phase, 1.f / 2048.f);
                if (std::isnan(output) || std::isinf(output))
                {
                    valid = false;
                    break;
                }
            }
            expect(valid, "Wavefolded output should be finite");
        }

        beginTest("Sync at 0 has no effect");
        {
            OscEffects fx;
            fx.setParams(0.f, 0.f, 0.5f, 0.f);
            fx.reset();

            // Run 100 samples - output should match input
            for (int i = 0; i < 100; ++i)
            {
                float phase = static_cast<float>(i) / 100.f;
                float input = std::sin(sdf::TWO_PI * phase);
                float output = fx.process(input, phase, 0.01f);
                expectWithinAbsoluteError(output, input, 0.001f);
            }
        }
    }
};

static OscEffectsTest oscEffectsTest;
