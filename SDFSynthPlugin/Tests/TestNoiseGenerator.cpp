#include <juce_core/juce_core.h>
#include "DSP/NoiseGenerator.h"
#include <cmath>

class NoiseGeneratorTest : public juce::UnitTest
{
public:
    NoiseGeneratorTest() : juce::UnitTest("NoiseGenerator") {}

    void runTest() override
    {
        beginTest("White noise: zero mean over 1M samples");
        {
            NoiseGenerator gen;
            gen.reset();
            gen.setSampleRate(44100.f);

            double sum = 0.0;
            constexpr int N = 1000000;
            for (int i = 0; i < N; ++i)
                sum += gen.nextSample(NoiseType::White);

            double mean = sum / N;
            expect(std::abs(mean) < 0.01, "White noise mean should be near zero");
        }

        beginTest("White noise: correct RMS (~0.577)");
        {
            NoiseGenerator gen;
            gen.reset();
            gen.setSampleRate(44100.f);

            double sumSq = 0.0;
            constexpr int N = 1000000;
            for (int i = 0; i < N; ++i)
            {
                float s = gen.nextSample(NoiseType::White);
                sumSq += s * s;
            }

            double rms = std::sqrt(sumSq / N);
            // Uniform [-1,1] RMS = 1/sqrt(3) ~ 0.577
            expect(std::abs(rms - 0.577) < 0.02, "White noise RMS should be ~0.577");
        }

        beginTest("No NaN/Inf over 1M samples (all types)");
        {
            NoiseGenerator gen;
            gen.reset();
            gen.setSampleRate(44100.f);

            constexpr int N = 1000000;
            bool valid = true;
            for (int i = 0; i < N; ++i)
            {
                float w = gen.nextSample(NoiseType::White);
                float p = gen.nextSample(NoiseType::Pink);
                float b = gen.nextSample(NoiseType::Brown);
                if (std::isnan(w) || std::isinf(w) ||
                    std::isnan(p) || std::isinf(p) ||
                    std::isnan(b) || std::isinf(b))
                {
                    valid = false;
                    break;
                }
            }
            expect(valid, "All noise samples should be finite");
        }

        beginTest("Pink noise has less high-frequency energy than white");
        {
            NoiseGenerator gen;
            gen.reset();
            gen.setSampleRate(44100.f);

            // Simple measure: sum of absolute differences (high-freq proxy)
            constexpr int N = 100000;
            double whiteDiffSum = 0.0, pinkDiffSum = 0.0;
            float prevW = 0.f, prevP = 0.f;
            for (int i = 0; i < N; ++i)
            {
                float w = gen.nextSample(NoiseType::White);
                float p = gen.nextSample(NoiseType::Pink);
                whiteDiffSum += std::abs(w - prevW);
                pinkDiffSum += std::abs(p - prevP);
                prevW = w;
                prevP = p;
            }
            expect(pinkDiffSum < whiteDiffSum, "Pink noise should have less high-freq energy");
        }
    }
};

static NoiseGeneratorTest noiseGenTest;
