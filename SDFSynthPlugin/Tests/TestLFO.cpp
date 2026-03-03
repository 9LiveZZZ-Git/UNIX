#include <juce_core/juce_core.h>
#include "DSP/LFO.h"
#include <cmath>

class LFOTest : public juce::UnitTest
{
public:
    LFOTest() : juce::UnitTest("LFO") {}

    void runTest() override
    {
        beginTest("Sine output range [-1, +1]");
        {
            LFO lfo;
            lfo.reset();
            lfo.setRate(5.f);
            lfo.setShape(LFOShape::Sine);

            float minVal = 1.f, maxVal = -1.f;
            for (int i = 0; i < 1000; ++i)
            {
                float v = lfo.advance(64, 44100.0);
                minVal = std::min(minVal, v);
                maxVal = std::max(maxVal, v);
            }
            expect(minVal >= -1.001f, "Sine min should be >= -1 (got " + juce::String(minVal) + ")");
            expect(maxVal <= 1.001f, "Sine max should be <= 1 (got " + juce::String(maxVal) + ")");
            expect(minVal < -0.9f, "Sine should reach near -1 (got " + juce::String(minVal) + ")");
            expect(maxVal > 0.9f, "Sine should reach near +1 (got " + juce::String(maxVal) + ")");
        }

        beginTest("Triangle output range [-1, +1]");
        {
            LFO lfo;
            lfo.reset();
            lfo.setRate(3.f);
            lfo.setShape(LFOShape::Triangle);

            float minVal = 1.f, maxVal = -1.f;
            for (int i = 0; i < 1000; ++i)
            {
                float v = lfo.advance(64, 44100.0);
                minVal = std::min(minVal, v);
                maxVal = std::max(maxVal, v);
            }
            expect(minVal >= -1.001f && maxVal <= 1.001f, "Triangle should be in [-1, 1]");
            expect(minVal < -0.9f && maxVal > 0.9f, "Triangle should reach near extremes");
        }

        beginTest("Saw PolyBLEP: no large discontinuities");
        {
            LFO lfo;
            lfo.reset();
            lfo.setRate(10.f);
            lfo.setShape(LFOShape::Saw);

            float prev = lfo.advance(1, 44100.0);
            int largeJumps = 0;
            for (int i = 0; i < 44100; ++i)
            {
                float v = lfo.advance(1, 44100.0);
                if (std::abs(v - prev) > 1.5f)
                    ++largeJumps;
                prev = v;
            }
            // PolyBLEP should smooth discontinuities; allow very few
            expect(largeJumps < 20, "Saw PolyBLEP should have few large jumps (got "
                   + juce::String(largeJumps) + ")");
        }

        beginTest("Square PolyBLEP: output roughly bipolar");
        {
            LFO lfo;
            lfo.reset();
            lfo.setRate(5.f);
            lfo.setShape(LFOShape::Square);

            int posCount = 0, negCount = 0;
            for (int i = 0; i < 10000; ++i)
            {
                float v = lfo.advance(1, 44100.0);
                if (v > 0.5f) ++posCount;
                else if (v < -0.5f) ++negCount;
            }
            expect(posCount > 1000, "Square should have many positive samples");
            expect(negCount > 1000, "Square should have many negative samples");
        }

        beginTest("S&H holds value between cycles");
        {
            LFO lfo;
            lfo.reset();
            lfo.setRate(1.f); // 1 Hz
            lfo.setShape(LFOShape::SandH);

            // Advance less than one cycle and check value stays constant
            float firstVal = lfo.advance(100, 44100.0);
            bool allSame = true;
            for (int i = 0; i < 100; ++i)
            {
                float v = lfo.advance(100, 44100.0);
                if (std::abs(v - firstVal) > 0.001f)
                    allSame = false;
            }
            // Within first cycle (< 44100 samples at 1 Hz), values should be constant
            // We've advanced 100 * 101 = 10100 samples out of 44100 = ~23% of cycle
            expect(allSame, "S&H should hold value within one cycle");
        }

        beginTest("S&H changes value after cycle wrap");
        {
            LFO lfo;
            lfo.reset();
            lfo.setRate(10.f); // 10 Hz = cycle every 4410 samples

            lfo.setShape(LFOShape::SandH);

            float firstVal = lfo.advance(1, 44100.0);
            // Advance through several full cycles
            bool changed = false;
            for (int i = 0; i < 200; ++i)
            {
                float v = lfo.advance(441, 44100.0); // ~0.1s per step, = 1 cycle
                if (std::abs(v - firstVal) > 0.001f)
                {
                    changed = true;
                    break;
                }
            }
            expect(changed, "S&H should change value after cycle wraps");
        }

        beginTest("Tempo sync: 1/4 note at 120 BPM = 2 Hz");
        {
            LFO lfo;
            lfo.reset();
            lfo.setShape(LFOShape::Sine);
            lfo.setSyncEnabled(true);
            lfo.setTempoSyncDivision(5); // 1/4 note

            // At 120 BPM, 1/4 note = 1 beat = 0.5 seconds
            // So LFO frequency = 2 Hz, period = 22050 samples at 44100
            // Run for exactly one period and check it completes a full cycle
            int samplesPerPeriod = 22050;
            int step = 1;
            float minVal = 1.f, maxVal = -1.f;
            for (int i = 0; i < samplesPerPeriod; ++i)
            {
                float v = lfo.advance(step, 44100.0, 120.0);
                minVal = std::min(minVal, v);
                maxVal = std::max(maxVal, v);
            }
            expect(minVal < -0.8f, "Sync'd sine should reach near -1 in one period");
            expect(maxVal > 0.8f, "Sync'd sine should reach near +1 in one period");
        }

        beginTest("Phase offset: 180 degrees inverts sine");
        {
            LFO lfo0;
            lfo0.reset();
            lfo0.setRate(5.f);
            lfo0.setShape(LFOShape::Sine);
            lfo0.setPhaseOffset(0.f);

            LFO lfo180;
            lfo180.reset();
            lfo180.setRate(5.f);
            lfo180.setShape(LFOShape::Sine);
            lfo180.setPhaseOffset(180.f);

            // After same number of advances, values should be roughly inverted
            float sum = 0.f;
            for (int i = 0; i < 100; ++i)
            {
                float v0 = lfo0.advance(64, 44100.0);
                float v180 = lfo180.advance(64, 44100.0);
                sum += v0 + v180;
            }
            // Sum of value + inverted value should be near 0
            float avg = sum / 100.f;
            expect(std::abs(avg) < 0.15f, "0 + 180 degree offset should roughly cancel (avg="
                   + juce::String(avg) + ")");
        }

        beginTest("No NaN/Inf across all shapes");
        {
            bool valid = true;
            for (int shapeIdx = 0; shapeIdx < 6; ++shapeIdx)
            {
                LFO lfo;
                lfo.reset();
                lfo.setRate(20.f);
                lfo.setShape(static_cast<LFOShape>(shapeIdx));

                for (int i = 0; i < 10000; ++i)
                {
                    float v = lfo.advance(32, 44100.0);
                    if (std::isnan(v) || std::isinf(v))
                    {
                        valid = false;
                        break;
                    }
                }
                if (!valid) break;
            }
            expect(valid, "All LFO shapes should produce finite values");
        }

        beginTest("Random shape produces smoothly varying output");
        {
            LFO lfo;
            lfo.reset();
            lfo.setRate(5.f);
            lfo.setShape(LFOShape::Random);

            float prev = lfo.advance(1, 44100.0);
            int largeJumps = 0;
            for (int i = 0; i < 44100; ++i)
            {
                float v = lfo.advance(1, 44100.0);
                if (std::abs(v - prev) > 0.5f)
                    ++largeJumps;
                prev = v;
            }
            // Smoothed random should have very few large jumps
            expect(largeJumps < 50, "Random shape should be smooth (large jumps: "
                   + juce::String(largeJumps) + ")");
        }
    }
};

static LFOTest lfoTest;
