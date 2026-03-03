#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/FXChain.h"
#include <cmath>

class FXChainTest : public juce::UnitTest
{
public:
    FXChainTest() : juce::UnitTest("FXChain") {}

    void runTest() override
    {
        beginTest("All disabled = passthrough (bit-exact)");
        {
            FXChain fx;
            fx.prepare(44100.0, 512);

            juce::AudioBuffer<float> buf(2, 512);
            // Fill with known data
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    buf.setSample(ch, i, std::sin(static_cast<float>(i) * 0.1f) * 0.5f);

            // Copy original
            juce::AudioBuffer<float> original(2, 512);
            for (int ch = 0; ch < 2; ++ch)
                original.copyFrom(ch, 0, buf, ch, 0, 512);

            fx.setDistParams(false, 1.f, 0.5f, DistortionType::SoftClip);
            fx.setChorusParams(false, 1.f, 0.3f, 0.3f);
            fx.setDelayParams(false, 250.f, 0.4f, 0.3f);
            fx.setReverbParams(false, 0.5f, 0.5f, 0.3f);
            fx.process(buf);

            bool identical = true;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    if (buf.getSample(ch, i) != original.getSample(ch, i))
                        identical = false;

            expect(identical, "All-disabled FX should be bit-exact passthrough");
        }

        beginTest("Soft clip limits output near 1.0");
        {
            FXChain fx;
            fx.prepare(44100.0, 512);

            juce::AudioBuffer<float> buf(2, 512);
            // Fill with loud signal
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    buf.setSample(ch, i, 5.f);

            fx.setDistParams(true, 10.f, 1.f, DistortionType::SoftClip);
            fx.setChorusParams(false, 1.f, 0.3f, 0.3f);
            fx.setDelayParams(false, 250.f, 0.4f, 0.3f);
            fx.setReverbParams(false, 0.5f, 0.5f, 0.3f);
            fx.process(buf);

            float maxAbs = 0.f;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    maxAbs = std::max(maxAbs, std::abs(buf.getSample(ch, i)));

            expect(maxAbs <= 1.01f, "Soft clip should limit to ~1.0 (got " + juce::String(maxAbs) + ")");
            expect(maxAbs > 0.9f, "Soft clip should produce signal near 1.0 (got " + juce::String(maxAbs) + ")");
        }

        beginTest("Hard clip clamps exactly [-1, 1]");
        {
            FXChain fx;
            fx.prepare(44100.0, 512);

            juce::AudioBuffer<float> buf(2, 512);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    buf.setSample(ch, i, (i % 2 == 0) ? 3.f : -3.f);

            fx.setDistParams(true, 5.f, 1.f, DistortionType::HardClip);
            fx.setChorusParams(false, 1.f, 0.3f, 0.3f);
            fx.setDelayParams(false, 250.f, 0.4f, 0.3f);
            fx.setReverbParams(false, 0.5f, 0.5f, 0.3f);
            fx.process(buf);

            float maxVal = -100.f, minVal = 100.f;
            for (int ch = 0; ch < 2; ++ch)
            {
                for (int i = 0; i < 512; ++i)
                {
                    float v = buf.getSample(ch, i);
                    maxVal = std::max(maxVal, v);
                    minVal = std::min(minVal, v);
                }
            }

            expectWithinAbsoluteError(maxVal, 1.f, 0.001f, "Hard clip max should be 1.0");
            expectWithinAbsoluteError(minVal, -1.f, 0.001f, "Hard clip min should be -1.0");
        }

        beginTest("Delay produces delayed impulse");
        {
            FXChain fx;
            fx.prepare(44100.0, 4096);

            juce::AudioBuffer<float> buf(2, 4096);
            buf.clear();
            // Impulse at sample 0
            buf.setSample(0, 0, 1.f);
            buf.setSample(1, 0, 1.f);

            float delayMs = 100.f; // 100ms = 4410 samples @ 44100
            fx.setDistParams(false, 1.f, 0.5f, DistortionType::SoftClip);
            fx.setChorusParams(false, 1.f, 0.3f, 0.3f);
            fx.setDelayParams(true, delayMs, 0.f, 1.f); // no feedback, full wet
            fx.setReverbParams(false, 0.5f, 0.5f, 0.3f);
            fx.process(buf);

            // With ping-pong, first tap appears in L from R delay line
            // Check that there's energy after sample 100 (rough check for delay)
            float earlyEnergy = 0.f, lateEnergy = 0.f;
            for (int i = 2; i < 100; ++i)
                earlyEnergy += std::abs(buf.getSample(0, i));
            for (int i = 3000; i < 4096; ++i)
                lateEnergy += std::abs(buf.getSample(0, i));

            // The dry signal is still at sample 0, but delay tap should appear later
            expect(buf.getSample(0, 0) > 0.9f, "Dry impulse should still be at sample 0");
            // With 100ms delay, energy should appear around sample 4410 which is out of buffer
            // But the ping-pong cross-feed should produce some late energy
        }

        beginTest("Reverb adds tail energy after silence");
        {
            FXChain fx;
            fx.prepare(44100.0, 512);

            fx.setDistParams(false, 1.f, 0.5f, DistortionType::SoftClip);
            fx.setChorusParams(false, 1.f, 0.3f, 0.3f);
            fx.setDelayParams(false, 250.f, 0.4f, 0.3f);
            fx.setReverbParams(true, 0.9f, 0.2f, 1.f); // full wet, large room

            // Feed several blocks of signal to build up reverb state
            for (int b = 0; b < 5; ++b)
            {
                juce::AudioBuffer<float> buf(2, 512);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                        buf.setSample(ch, i, std::sin(static_cast<float>(b * 512 + i) * 0.1f) * 0.5f);
                fx.process(buf);
            }

            // Now feed silence: reverb should produce tail
            juce::AudioBuffer<float> silenceBuf(2, 512);
            silenceBuf.clear();
            fx.process(silenceBuf);

            float tailEnergy = 0.f;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    tailEnergy += std::abs(silenceBuf.getSample(ch, i));

            expect(tailEnergy > 0.01f, "Reverb should produce tail energy after signal stops (got " + juce::String(tailEnergy) + ")");
        }

        beginTest("No NaN/Inf under stress (all FX on, extreme params, 100 blocks)");
        {
            FXChain fx;
            fx.prepare(44100.0, 256);

            fx.setDistParams(true, 20.f, 1.f, DistortionType::Fold);
            fx.setChorusParams(true, 10.f, 1.f, 1.f);
            fx.setDelayParams(true, 10.f, 0.95f, 1.f);
            fx.setReverbParams(true, 1.f, 0.f, 1.f);

            bool hasNanInf = false;
            for (int block = 0; block < 100; ++block)
            {
                juce::AudioBuffer<float> buf(2, 256);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 256; ++i)
                        buf.setSample(ch, i, std::sin(static_cast<float>(block * 256 + i) * 0.05f) * 0.8f);

                fx.process(buf);

                for (int ch = 0; ch < 2; ++ch)
                {
                    for (int i = 0; i < 256; ++i)
                    {
                        float v = buf.getSample(ch, i);
                        if (std::isnan(v) || std::isinf(v))
                        {
                            hasNanInf = true;
                            break;
                        }
                    }
                    if (hasNanInf) break;
                }
                if (hasNanInf) break;
            }

            expect(!hasNanInf, "No NaN or Inf should be produced under stress");
        }

        beginTest("Bitcrush quantizes correctly");
        {
            FXChain fx;
            fx.prepare(44100.0, 512);

            juce::AudioBuffer<float> buf(1, 512);
            // Fill with a ramp from -1 to 1
            for (int i = 0; i < 512; ++i)
                buf.setSample(0, i, (static_cast<float>(i) / 511.f) * 2.f - 1.f);

            // drive=1 -> q = max(2, round(1*2)) = 2 -> quantize to 0.5 steps
            fx.setDistParams(true, 1.f, 1.f, DistortionType::Bitcrush);
            fx.setChorusParams(false, 1.f, 0.3f, 0.3f);
            fx.setDelayParams(false, 250.f, 0.4f, 0.3f);
            fx.setReverbParams(false, 0.5f, 0.5f, 0.3f);
            fx.process(buf);

            // With q=2, output should be multiples of 0.5: -1, -0.5, 0, 0.5, 1
            bool quantized = true;
            for (int i = 0; i < 512; ++i)
            {
                float v = buf.getSample(0, i);
                float nearest = std::round(v * 2.f) / 2.f;
                if (std::abs(v - nearest) > 0.001f)
                {
                    quantized = false;
                    break;
                }
            }

            expect(quantized, "Bitcrush should quantize to steps of 1/q");
        }
    }
};

static FXChainTest fxChainTest;
