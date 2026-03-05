#include <juce_core/juce_core.h>
#include "DSP/SVFFilter.h"
#include <cmath>
#include <vector>

class FilterModesTest : public juce::UnitTest
{
public:
    FilterModesTest() : juce::UnitTest("FilterModes") {}

    // Generate a sine wave buffer at a given frequency
    static std::vector<float> makeSine(float freqHz, float sampleRate, int numSamples)
    {
        std::vector<float> buf(static_cast<size_t>(numSamples));
        for (int i = 0; i < numSamples; ++i)
            buf[static_cast<size_t>(i)] = std::sin(sdf::TWO_PI * freqHz * static_cast<float>(i) / sampleRate);
        return buf;
    }

    // Measure RMS of a signal passed through an SVF with given mode and cutoff
    static float measureFilteredRMS(const std::vector<float>& input, FilterMode mode,
                                    float cutoffHz, float resonance, float sampleRate)
    {
        SVFFilter f;
        f.reset();
        f.setParams(cutoffHz, resonance, sampleRate);
        f.setMode(mode);

        // Warm up filter for 512 samples to let coefficients settle
        for (int i = 0; i < 512; ++i)
            f.process(0.f);

        double sumSq = 0.0;
        int count = 0;
        for (size_t i = 0; i < input.size(); ++i)
        {
            float out = f.process(input[i]);
            // Skip first 512 samples of signal for transient settling
            if (i >= 512)
            {
                sumSq += static_cast<double>(out) * static_cast<double>(out);
                ++count;
            }
        }
        return static_cast<float>(std::sqrt(sumSq / count));
    }

    void runTest() override
    {
        constexpr float sr = 44100.f;
        constexpr int N = 4096;
        constexpr float cutoff = 2000.f;
        constexpr float res = 0.1f;

        auto dcSignal = std::vector<float>(N, 1.f);
        auto lowSine  = makeSine(100.f, sr, N);
        auto midSine  = makeSine(2000.f, sr, N);
        auto highSine = makeSine(15000.f, sr, N);

        // ── LOW PASS ────────────────────────────────────────────────
        beginTest("LowPass: passes DC");
        {
            float rms = measureFilteredRMS(dcSignal, FilterMode::LowPass, cutoff, res, sr);
            expect(rms > 0.9f, "LP should pass DC (rms=" + juce::String(rms) + ")");
        }

        beginTest("LowPass: passes 100Hz");
        {
            float rms = measureFilteredRMS(lowSine, FilterMode::LowPass, cutoff, res, sr);
            expect(rms > 0.5f, "LP should pass 100Hz (rms=" + juce::String(rms) + ")");
        }

        beginTest("LowPass: attenuates 15kHz");
        {
            float rms = measureFilteredRMS(highSine, FilterMode::LowPass, cutoff, res, sr);
            expect(rms < 0.1f, "LP should attenuate 15kHz (rms=" + juce::String(rms) + ")");
        }

        // ── HIGH PASS ───────────────────────────────────────────────
        beginTest("HighPass: rejects DC");
        {
            float rms = measureFilteredRMS(dcSignal, FilterMode::HighPass, cutoff, res, sr);
            expect(rms < 0.05f, "HP should reject DC (rms=" + juce::String(rms) + ")");
        }

        beginTest("HighPass: attenuates 100Hz");
        {
            float rms = measureFilteredRMS(lowSine, FilterMode::HighPass, cutoff, res, sr);
            expect(rms < 0.15f, "HP should attenuate 100Hz (rms=" + juce::String(rms) + ")");
        }

        beginTest("HighPass: passes 15kHz");
        {
            float rms = measureFilteredRMS(highSine, FilterMode::HighPass, cutoff, res, sr);
            expect(rms > 0.4f, "HP should pass 15kHz (rms=" + juce::String(rms) + ")");
        }

        // ── BAND PASS ──────────────────────────────────────────────
        beginTest("BandPass: attenuates DC");
        {
            float rms = measureFilteredRMS(dcSignal, FilterMode::BandPass, cutoff, res, sr);
            expect(rms < 0.05f, "BP should reject DC (rms=" + juce::String(rms) + ")");
        }

        beginTest("BandPass: passes near-cutoff frequency");
        {
            float rms = measureFilteredRMS(midSine, FilterMode::BandPass, cutoff, res, sr);
            expect(rms > 0.2f, "BP should pass 2kHz at cutoff=2kHz (rms=" + juce::String(rms) + ")");
        }

        beginTest("BandPass: attenuates 15kHz");
        {
            float rms = measureFilteredRMS(highSine, FilterMode::BandPass, cutoff, res, sr);
            expect(rms < 0.15f, "BP should attenuate 15kHz (rms=" + juce::String(rms) + ")");
        }

        // ── NOTCH ──────────────────────────────────────────────────
        beginTest("Notch: passes DC");
        {
            float rms = measureFilteredRMS(dcSignal, FilterMode::Notch, cutoff, res, sr);
            expect(rms > 0.9f, "Notch should pass DC (rms=" + juce::String(rms) + ")");
        }

        beginTest("Notch: rejects near-cutoff frequency");
        {
            float rmsNotch = measureFilteredRMS(midSine, FilterMode::Notch, cutoff, res, sr);
            float rmsLP    = measureFilteredRMS(midSine, FilterMode::LowPass, cutoff, res, sr);
            expect(rmsNotch < rmsLP, "Notch should attenuate 2kHz more than LP at cutoff=2kHz");
        }

        beginTest("Notch: passes 15kHz");
        {
            float rms = measureFilteredRMS(highSine, FilterMode::Notch, cutoff, res, sr);
            expect(rms > 0.4f, "Notch should pass 15kHz (rms=" + juce::String(rms) + ")");
        }

        // ── PEAK ───────────────────────────────────────────────────
        beginTest("Peak: LP and HP regions cancel");
        {
            // Peak = LP - HP; for DC, LP passes and HP rejects → Peak passes DC
            float rms = measureFilteredRMS(dcSignal, FilterMode::Peak, cutoff, res, sr);
            expect(rms > 0.8f, "Peak should pass DC (rms=" + juce::String(rms) + ")");
        }

        beginTest("Peak: high frequencies inverted vs low");
        {
            // Peak = v2 - (v0 - k*v1 - v2) = 2*LP - HP
            // For high freq: LP≈0, HP≈signal → Peak ≈ -signal → RMS still positive
            float rms = measureFilteredRMS(highSine, FilterMode::Peak, cutoff, res, sr);
            expect(rms > 0.3f, "Peak should have significant output at 15kHz (rms=" + juce::String(rms) + ")");
        }

        // ── STABILITY ──────────────────────────────────────────────
        beginTest("All modes: no NaN/Inf with impulse input");
        {
            bool valid = true;
            FilterMode modes[] = { FilterMode::LowPass, FilterMode::BandPass,
                                   FilterMode::HighPass, FilterMode::Notch, FilterMode::Peak };
            for (auto mode : modes)
            {
                SVFFilter f;
                f.reset();
                f.setParams(cutoff, res, sr);
                f.setMode(mode);

                // Impulse: 1.0 followed by zeros
                float out = f.process(1.f);
                if (std::isnan(out) || std::isinf(out)) valid = false;

                for (int i = 0; i < 2000; ++i)
                {
                    out = f.process(0.f);
                    if (std::isnan(out) || std::isinf(out))
                    {
                        valid = false;
                        break;
                    }
                }
                if (!valid) break;
            }
            expect(valid, "All filter modes should produce finite output from impulse");
        }

        beginTest("All modes: stable at extreme cutoff values");
        {
            bool valid = true;
            float extremeCutoffs[] = { 20.f, 100.f, 1000.f, 10000.f, 20000.f };
            FilterMode modes[] = { FilterMode::LowPass, FilterMode::BandPass,
                                   FilterMode::HighPass, FilterMode::Notch, FilterMode::Peak };

            for (auto cutHz : extremeCutoffs)
            {
                for (auto mode : modes)
                {
                    SVFFilter f;
                    f.reset();
                    f.setParams(cutHz, 0.9f, sr);  // high resonance
                    f.setMode(mode);

                    for (int i = 0; i < 1000; ++i)
                    {
                        float input = std::sin(sdf::TWO_PI * 440.f * static_cast<float>(i) / sr);
                        float out = f.process(input);
                        if (std::isnan(out) || std::isinf(out))
                        {
                            valid = false;
                            break;
                        }
                    }
                    if (!valid) break;
                }
                if (!valid) break;
            }
            expect(valid, "All modes should be stable at extreme cutoff + high resonance");
        }

        beginTest("Mode crossfade: no large jumps when switching modes");
        {
            SVFFilter f;
            f.reset();
            f.setParams(2000.f, 0.3f, sr);
            f.setMode(FilterMode::LowPass);

            // Process some signal in LP mode
            float prev = 0.f;
            for (int i = 0; i < 500; ++i)
            {
                float input = std::sin(sdf::TWO_PI * 500.f * static_cast<float>(i) / sr);
                prev = f.process(input);
            }

            // Switch to HP mode mid-stream
            f.setMode(FilterMode::HighPass);

            float maxJump = 0.f;
            for (int i = 500; i < 1500; ++i)
            {
                float input = std::sin(sdf::TWO_PI * 500.f * static_cast<float>(i) / sr);
                float out = f.process(input);
                float jump = std::abs(out - prev);
                if (jump > maxJump) maxJump = jump;
                prev = out;
            }
            // With crossfade, there shouldn't be any single-sample jumps > 0.5
            expect(maxJump < 0.5f, "Mode switch should crossfade smoothly (maxJump="
                   + juce::String(maxJump) + ")");
        }
    }
};

static FilterModesTest filterModesTest;
