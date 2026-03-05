#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "DSP/SDFSynthesiser.h"
#include "DSP/SDFVoice.h"
#include "DSP/WavetableGenerator.h"
#include "DSP/SVFFilter.h"
#include "DSP/FXChain.h"
#include <cmath>
#include <memory>

// Click/noise detection tests — renders audio through the synth pipeline
// and checks for discontinuities, DC offset, NaN, and excessive peaks.
// All large objects (MipMappedWavetable, SDFSynthesiser) are heap-allocated
// to avoid stack overflow (~88KB per MipMappedWavetable).
//
// NOTE: Saw waves naturally have ~2.0 jumps per cycle (wrap from +1 to -1).
// Tests using saw waveforms use higher thresholds. Tests using sine waveforms
// can detect true discontinuities with low thresholds.

class ClickDetectionTest : public juce::UnitTest
{
public:
    ClickDetectionTest() : juce::UnitTest("ClickDetection") {}

    static std::shared_ptr<const MipMappedWavetable> makeSineMip()
    {
        WavetableGenerator::Wavetable wt{};
        for (int i = 0; i < sdf::TABLE_SIZE; ++i)
            wt[i] = std::sin(sdf::TWO_PI * static_cast<float>(i) / sdf::TABLE_SIZE);
        return std::make_shared<const MipMappedWavetable>(WavetableGenerator::generateMipMap(wt));
    }

    static std::shared_ptr<const MipMappedWavetable> makeSawMip()
    {
        WavetableGenerator::Wavetable wt{};
        for (int i = 0; i < sdf::TABLE_SIZE; ++i)
            wt[i] = 2.f * static_cast<float>(i) / sdf::TABLE_SIZE - 1.f;
        return std::make_shared<const MipMappedWavetable>(WavetableGenerator::generateMipMap(wt));
    }

    // Second sine wave at different phase for crossfade testing
    static std::shared_ptr<const MipMappedWavetable> makeSine2Mip()
    {
        WavetableGenerator::Wavetable wt{};
        for (int i = 0; i < sdf::TABLE_SIZE; ++i)
        {
            // Slightly different harmonic content (sine + soft 2nd harmonic)
            float t = static_cast<float>(i) / sdf::TABLE_SIZE;
            wt[i] = std::sin(sdf::TWO_PI * t) * 0.8f
                   + std::sin(sdf::TWO_PI * t * 2.f) * 0.2f;
        }
        return std::make_shared<const MipMappedWavetable>(WavetableGenerator::generateMipMap(wt));
    }

    struct ClickReport
    {
        int clickCount = 0;
        float maxJump = 0.f;
        int maxJumpSample = 0;
        float dcOffset = 0.f;
        float peakLevel = 0.f;
        float rmsLevel = 0.f;
        bool hasNaN = false;
        bool hasInf = false;
    };

    static ClickReport analyzeBuffer(const juce::AudioBuffer<float>& buf, int ch,
                                      float clickThreshold = 0.3f,
                                      int skipSamples = 32)
    {
        ClickReport report;
        int n = buf.getNumSamples();
        const float* data = buf.getReadPointer(ch);

        double sum = 0.0, sumSq = 0.0;

        for (int i = 0; i < n; ++i)
        {
            float s = data[i];
            if (std::isnan(s)) { report.hasNaN = true; continue; }
            if (std::isinf(s)) { report.hasInf = true; continue; }

            sum += s;
            sumSq += static_cast<double>(s) * s;
            float absS = std::abs(s);
            if (absS > report.peakLevel) report.peakLevel = absS;

            if (i > skipSamples)
            {
                float jump = std::abs(s - data[i - 1]);
                if (jump > report.maxJump)
                {
                    report.maxJump = jump;
                    report.maxJumpSample = i;
                }
                if (jump > clickThreshold)
                    ++report.clickCount;
            }
        }

        report.dcOffset = static_cast<float>(sum / n);
        report.rmsLevel = static_cast<float>(std::sqrt(sumSq / n));
        return report;
    }

    void runTest() override
    {
        auto sineMip = makeSineMip();
        auto sine2Mip = makeSine2Mip();
        auto sawMip = makeSawMip();

        // ── TEST 1: Single note sine — no clicks ───────────────────────
        beginTest("Single note sine — no clicks in output");
        {
            auto synth = std::make_unique<SDFSynthesiser>();
            synth->setCurrentPlaybackSampleRate(44100.0);
            synth->setMipMappedWavetable(sineMip);
            synth->updateADSR(0.01f, 0.1f, 0.8f, 0.3f);
            synth->updateUnisonParams(1, 20.f, 50.f, 0.5f);

            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);

            juce::AudioBuffer<float> buf(2, 8192);
            buf.clear();
            synth->renderNextBlock(buf, midi, 0, 8192);

            auto report = analyzeBuffer(buf, 0, 0.15f, 64);
            expect(!report.hasNaN, "No NaN in single note output");
            expect(!report.hasInf, "No Inf in single note output");
            expect(report.clickCount == 0,
                   "Single sine note should have no clicks (found "
                   + juce::String(report.clickCount) + ", maxJump="
                   + juce::String(report.maxJump, 4) + " at sample "
                   + juce::String(report.maxJumpSample) + ")");
            expect(report.rmsLevel > 0.001f, "Should produce audible output");
        }

        // ── TEST 2: Note-on then note-off, clean release ───────────────
        beginTest("Note-off produces clean release (no click)");
        {
            auto synth = std::make_unique<SDFSynthesiser>();
            synth->setCurrentPlaybackSampleRate(44100.0);
            synth->setMipMappedWavetable(sineMip);
            synth->updateADSR(0.01f, 0.05f, 0.8f, 0.5f);

            juce::MidiBuffer midi1;
            midi1.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            juce::AudioBuffer<float> buf1(2, 4096);
            buf1.clear();
            synth->renderNextBlock(buf1, midi1, 0, 4096);

            juce::MidiBuffer midi2;
            midi2.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 0);
            juce::AudioBuffer<float> buf2(2, 16384);
            buf2.clear();
            synth->renderNextBlock(buf2, midi2, 0, 16384);

            auto report = analyzeBuffer(buf2, 0, 0.15f, 0);
            expect(!report.hasNaN, "No NaN in release");
            expect(report.clickCount == 0,
                   "Release should have no clicks (found "
                   + juce::String(report.clickCount) + ", maxJump="
                   + juce::String(report.maxJump, 4) + ")");
        }

        // ── TEST 3: Voice stealing retrigger — no large click ──────────
        beginTest("Voice stealing retrigger — no large click");
        {
            auto synth = std::make_unique<SDFSynthesiser>();
            synth->setCurrentPlaybackSampleRate(44100.0);
            synth->setMipMappedWavetable(sineMip);
            synth->updateADSR(0.01f, 0.1f, 0.8f, 0.3f);

            juce::MidiBuffer midi1;
            midi1.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            juce::AudioBuffer<float> buf1(2, 2048);
            buf1.clear();
            synth->renderNextBlock(buf1, midi1, 0, 2048);

            juce::MidiBuffer midi2;
            midi2.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            juce::AudioBuffer<float> buf2(2, 4096);
            buf2.clear();
            synth->renderNextBlock(buf2, midi2, 0, 4096);

            auto report = analyzeBuffer(buf2, 0, 0.4f, 0);
            expect(!report.hasNaN, "No NaN after voice steal");
            expect(report.clickCount == 0,
                   "Voice steal should be click-free (found "
                   + juce::String(report.clickCount) + ", maxJump="
                   + juce::String(report.maxJump, 4) + ")");
        }

        // ── TEST 4: Sine wavetable crossfade — smooth boundary ─────────
        // Uses sine→sine2 (both smooth waveforms) so any jump >0.15 is a real bug
        beginTest("Sine wavetable crossfade — smooth transition");
        {
            auto synth = std::make_unique<SDFSynthesiser>();
            synth->setCurrentPlaybackSampleRate(44100.0);
            synth->setMipMappedWavetable(sineMip);
            synth->updateADSR(0.001f, 0.05f, 0.9f, 0.3f);

            juce::MidiBuffer midi1;
            midi1.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            juce::AudioBuffer<float> buf1(2, 2048);
            buf1.clear();
            synth->renderNextBlock(buf1, midi1, 0, 2048);

            float preSwitchLast = buf1.getSample(0, 2047);

            // Crossfade from sine to sine2 over 64 samples
            synth->setMipMappedWavetable(sine2Mip, 64);

            juce::MidiBuffer emptyMidi;
            juce::AudioBuffer<float> buf2(2, 2048);
            buf2.clear();
            synth->renderNextBlock(buf2, emptyMidi, 0, 2048);

            float postSwitchFirst = buf2.getSample(0, 0);
            float boundaryJump = std::abs(postSwitchFirst - preSwitchLast);

            auto report = analyzeBuffer(buf2, 0, 0.15f, 0);
            expect(!report.hasNaN, "No NaN during wavetable crossfade");
            expect(boundaryJump < 0.05f,
                   "Crossfade boundary should be smooth (jump="
                   + juce::String(boundaryJump, 6) + ")");
            expect(report.clickCount == 0,
                   "Sine crossfade should have no clicks (found "
                   + juce::String(report.clickCount) + ", maxJump="
                   + juce::String(report.maxJump, 4) + " at sample "
                   + juce::String(report.maxJumpSample) + ")");
        }

        // ── TEST 5: Polyphonic — 8 voices with unison, no NaN/Inf ─────
        beginTest("8-voice polyphonic with unison — no NaN/Inf");
        {
            auto synth = std::make_unique<SDFSynthesiser>();
            synth->setCurrentPlaybackSampleRate(44100.0);
            synth->setMipMappedWavetable(sawMip);
            synth->updateADSR(0.01f, 0.1f, 0.8f, 0.3f);
            synth->updateUnisonParams(4, 15.f, 80.f, 0.5f);

            juce::MidiBuffer midi;
            int notes[] = {48, 52, 55, 60, 64, 67, 72, 76};
            for (int i = 0; i < 8; ++i)
                midi.addEvent(juce::MidiMessage::noteOn(1, notes[i], (juce::uint8)90), i * 10);

            juce::AudioBuffer<float> buf(2, 8192);
            buf.clear();
            synth->renderNextBlock(buf, midi, 0, 8192);

            auto report = analyzeBuffer(buf, 0, 0.5f, 128);
            expect(!report.hasNaN, "No NaN in 8-voice polyphony");
            expect(!report.hasInf, "No Inf in 8-voice polyphony");
            expect(report.peakLevel < 10.f,
                   "Peak should be bounded (peak=" + juce::String(report.peakLevel, 2) + ")");
        }

        // ── TEST 6: Silence after all notes off — no residual noise ────
        beginTest("Silence after all notes off — no residual output");
        {
            auto synth = std::make_unique<SDFSynthesiser>();
            synth->setCurrentPlaybackSampleRate(44100.0);
            synth->setMipMappedWavetable(sineMip);
            synth->updateADSR(0.001f, 0.05f, 0.8f, 0.1f);

            juce::MidiBuffer midi1;
            midi1.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            juce::AudioBuffer<float> buf1(2, 2048);
            buf1.clear();
            synth->renderNextBlock(buf1, midi1, 0, 2048);

            juce::MidiBuffer midi2;
            midi2.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 0);
            juce::AudioBuffer<float> buf2(2, 44100);
            buf2.clear();
            synth->renderNextBlock(buf2, midi2, 0, 44100);

            juce::MidiBuffer emptyMidi;
            juce::AudioBuffer<float> buf3(2, 4096);
            buf3.clear();
            synth->renderNextBlock(buf3, emptyMidi, 0, 4096);

            auto report = analyzeBuffer(buf3, 0, 0.01f, 0);
            expect(!report.hasNaN, "No NaN in silence");
            expect(report.rmsLevel < 0.001f,
                   "Should be nearly silent after release (rms="
                   + juce::String(report.rmsLevel, 6) + ")");
            expect(std::abs(report.dcOffset) < 0.001f,
                   "No DC offset in silence (dc="
                   + juce::String(report.dcOffset, 6) + ")");
        }

        // ── TEST 7: SVF filter — no click on parameter change ──────────
        // Uses sine input (smooth) so any discontinuity is a real filter bug
        beginTest("SVF filter sine input — no click on cutoff sweep");
        {
            SVFFilter filt;
            filt.reset();
            filt.setParams(1000.f, 0.3f, 44100.f);
            filt.setMode(FilterMode::LowPass);

            juce::AudioBuffer<float> buf(1, 8192);
            float phase = 0.f;
            float phaseInc = 440.f / 44100.f;
            for (int i = 0; i < 8192; ++i)
            {
                float sine = std::sin(sdf::TWO_PI * phase) * 0.5f;
                float cutoff = 200.f + (10000.f - 200.f) * static_cast<float>(i) / 8192.f;
                filt.setParams(cutoff, 0.3f, 44100.f);
                buf.setSample(0, i, filt.process(sine));
                phase += phaseInc;
                if (phase >= 1.f) phase -= 1.f;
            }

            auto report = analyzeBuffer(buf, 0, 0.15f, 64);
            expect(!report.hasNaN, "No NaN during filter sweep");
            expect(report.clickCount == 0,
                   "Filter sweep on sine should have no clicks (found "
                   + juce::String(report.clickCount) + ", maxJump="
                   + juce::String(report.maxJump, 4) + " at sample "
                   + juce::String(report.maxJumpSample) + ")");
        }

        // ── TEST 8: Multiple sine wavetable switches — no cumulative clicks
        beginTest("Multiple sine wavetable switches — no cumulative clicks");
        {
            auto synth = std::make_unique<SDFSynthesiser>();
            synth->setCurrentPlaybackSampleRate(44100.0);
            synth->setMipMappedWavetable(sineMip);
            synth->updateADSR(0.001f, 0.05f, 0.9f, 0.3f);

            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);

            juce::AudioBuffer<float> buf(2, 1024);
            buf.clear();
            synth->renderNextBlock(buf, midi, 0, 1024);

            int totalClicks = 0;
            float maxJumpAll = 0.f;
            juce::MidiBuffer empty;
            for (int sw = 0; sw < 10; ++sw)
            {
                // Alternate between sine and sine2 (both smooth)
                synth->setMipMappedWavetable((sw % 2 == 0) ? sine2Mip : sineMip, 64);

                juce::AudioBuffer<float> switchBuf(2, 512);
                switchBuf.clear();
                synth->renderNextBlock(switchBuf, empty, 0, 512);

                auto report = analyzeBuffer(switchBuf, 0, 0.15f, 0);
                totalClicks += report.clickCount;
                if (report.maxJump > maxJumpAll) maxJumpAll = report.maxJump;
                expect(!report.hasNaN, "No NaN during switch " + juce::String(sw));
            }

            expect(totalClicks == 0,
                   "Sine wavetable switches should have no clicks (found "
                   + juce::String(totalClicks) + " total, maxJump="
                   + juce::String(maxJumpAll, 4) + ")");
        }
    }
};

static ClickDetectionTest clickDetectionTest;
