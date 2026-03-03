#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "DSP/SDFSynthesiser.h"
#include "DSP/WavetableGenerator.h"
#include "DSP/NoiseGenerator.h"
#include "DSP/SVFFilter.h"
#include <cmath>
#include <memory>

// Integration tests exercising preset-like Phase 2 feature combinations
// through the full SDFSynthesiser pipeline.

class Phase2IntegrationTest : public juce::UnitTest
{
public:
    Phase2IntegrationTest() : juce::UnitTest("Phase2Integration") {}

    // Helper: create a sine mip-mapped wavetable
    static std::shared_ptr<const MipMappedWavetable> makeSineMip()
    {
        WavetableGenerator::Wavetable wt{};
        for (int i = 0; i < sdf::TABLE_SIZE; ++i)
            wt[i] = std::sin(sdf::TWO_PI * static_cast<float>(i) / sdf::TABLE_SIZE);
        return std::make_shared<const MipMappedWavetable>(WavetableGenerator::generateMipMap(wt));
    }

    // Helper: create a richer harmonic wavetable (saw-like)
    static std::shared_ptr<const MipMappedWavetable> makeSawMip()
    {
        WavetableGenerator::Wavetable wt{};
        for (int i = 0; i < sdf::TABLE_SIZE; ++i)
        {
            float phase = static_cast<float>(i) / sdf::TABLE_SIZE;
            wt[i] = 2.f * phase - 1.f; // naive saw for testing
        }
        return std::make_shared<const MipMappedWavetable>(WavetableGenerator::generateMipMap(wt));
    }

    // Helper: configure synth, play a note, return stereo buffer
    struct SynthConfig
    {
        int unisonVoices = 1;
        float detune = 20.f, spread = 50.f, blend = 0.5f;
        float fold = 0.f, pd = 0.f, pw = 0.5f, sync = 0.f;
        bool oscBEnable = false;
        float oscBLevel = 0.5f;
        int oscBSemi = 0;
        float oscBFine = 0.f;
        int oscBMixMode = 0;
        float oscBFMDepth = 0.f;
        float attack = 0.01f, decay = 0.1f, sustain = 0.8f, release = 0.3f;
        int midiNote = 60;
        int numSamples = 2048;
    };

    static juce::AudioBuffer<float> renderWithConfig(const SynthConfig& cfg,
                                                      std::shared_ptr<const MipMappedWavetable> mipA,
                                                      std::shared_ptr<const MipMappedWavetable> mipB)
    {
        SDFSynthesiser synth;
        synth.setCurrentPlaybackSampleRate(44100.0);
        synth.setMipMappedWavetable(mipA);
        synth.setMipMappedWavetableB(mipB);

        synth.updateUnisonParams(cfg.unisonVoices, cfg.detune, cfg.spread, cfg.blend);
        synth.updateOscEffects(cfg.fold, cfg.pd, cfg.pw, cfg.sync);
        synth.updateOscBParams(cfg.oscBEnable, cfg.oscBLevel, cfg.oscBSemi,
                               cfg.oscBFine, cfg.oscBMixMode, cfg.oscBFMDepth);
        synth.updateADSR(cfg.attack, cfg.decay, cfg.sustain, cfg.release);

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, cfg.midiNote, (juce::uint8)100), 0);

        juce::AudioBuffer<float> buf(2, cfg.numSamples);
        buf.clear();
        synth.renderNextBlock(buf, midi, 0, cfg.numSamples);
        return buf;
    }

    // Helper: check buffer for NaN/Inf
    static bool isBufferFinite(const juce::AudioBuffer<float>& buf)
    {
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            for (int i = 0; i < buf.getNumSamples(); ++i)
                if (std::isnan(buf.getSample(ch, i)) || std::isinf(buf.getSample(ch, i)))
                    return false;
        return true;
    }

    // Helper: compute difference between two buffers (sum of absolute diffs)
    static float bufferDiff(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        jassert(a.getNumSamples() == b.getNumSamples());
        float diff = 0.f;
        for (int i = 0; i < a.getNumSamples(); ++i)
            diff += std::abs(a.getSample(0, i) - b.getSample(0, i));
        return diff;
    }

    void runTest() override
    {
        auto sineMip = makeSineMip();
        auto sawMip  = makeSawMip();

        // ── ALL 4 OSC B MIX MODES PRODUCE DISTINCT OUTPUT ──────────
        beginTest("All 4 Osc B mix modes produce different output");
        {
            SynthConfig base;
            base.oscBEnable = true;
            base.oscBLevel = 0.5f;
            base.oscBSemi = 7;  // perfect 5th
            base.unisonVoices = 1;
            base.numSamples = 2048;

            juce::AudioBuffer<float> outputs[4];
            for (int mode = 0; mode < 4; ++mode)
            {
                base.oscBMixMode = mode;
                base.oscBFMDepth = (mode == 2) ? 0.3f : 0.f; // FM needs depth
                outputs[mode] = renderWithConfig(base, sineMip, sineMip);
            }

            // Each pair of modes should produce different output
            int distinctPairs = 0;
            for (int i = 0; i < 4; ++i)
                for (int j = i + 1; j < 4; ++j)
                    if (bufferDiff(outputs[i], outputs[j]) > 0.1f)
                        ++distinctPairs;

            // 6 pairs total (4 choose 2); all should differ
            expect(distinctPairs == 6, "All 6 pairs of Osc B mix modes should differ (got "
                   + juce::String(distinctPairs) + "/6)");
        }

        // ── FM DEPTH CHANGES SPECTRAL CONTENT ──────────────────────
        beginTest("FM depth > 0 adds spectral complexity");
        {
            SynthConfig noFM;
            noFM.oscBEnable = true;
            noFM.oscBLevel = 0.5f;
            noFM.oscBSemi = 7;
            noFM.oscBMixMode = 2; // FM
            noFM.oscBFMDepth = 0.f;

            SynthConfig withFM = noFM;
            withFM.oscBFMDepth = 0.4f;

            auto bufNoFM   = renderWithConfig(noFM, sineMip, sineMip);
            auto bufWithFM = renderWithConfig(withFM, sineMip, sineMip);

            // FM should produce different waveform
            float diff = bufferDiff(bufNoFM, bufWithFM);
            expect(diff > 0.1f, "FM depth should change output (diff=" + juce::String(diff) + ")");
            expect(isBufferFinite(bufWithFM), "FM output should be finite");
        }

        // ── WAVEFOLDING CHANGES SPECTRAL CONTENT ───────────────────
        beginTest("Wavefolding adds harmonics");
        {
            SynthConfig noFold;
            noFold.fold = 0.f;

            SynthConfig withFold;
            withFold.fold = 0.7f;

            auto bufClean  = renderWithConfig(noFold, sineMip, sineMip);
            auto bufFolded = renderWithConfig(withFold, sineMip, sineMip);

            float diff = bufferDiff(bufClean, bufFolded);
            expect(diff > 0.1f, "Wavefolding should change output (diff=" + juce::String(diff) + ")");
            expect(isBufferFinite(bufFolded), "Folded output should be finite");
        }

        // ── PHASE DISTORTION CHANGES OUTPUT ────────────────────────
        beginTest("Phase distortion changes timbral content");
        {
            SynthConfig noPD;
            noPD.pd = 0.f;

            SynthConfig withPD;
            withPD.pd = 0.4f;

            auto bufClean = renderWithConfig(noPD, sineMip, sineMip);
            auto bufPD    = renderWithConfig(withPD, sineMip, sineMip);

            float diff = bufferDiff(bufClean, bufPD);
            expect(diff > 0.1f, "Phase distortion should change output (diff=" + juce::String(diff) + ")");
            expect(isBufferFinite(bufPD), "PD output should be finite");
        }

        // ── PULSE WIDTH CHANGES OUTPUT ─────────────────────────────
        beginTest("Pulse width != 0.5 changes output");
        {
            SynthConfig center;
            center.pw = 0.5f;

            SynthConfig narrow;
            narrow.pw = 0.2f;

            auto bufCenter = renderWithConfig(center, sawMip, sawMip);
            auto bufNarrow = renderWithConfig(narrow, sawMip, sawMip);

            float diff = bufferDiff(bufCenter, bufNarrow);
            expect(diff > 0.1f, "PW=0.2 should differ from PW=0.5 (diff=" + juce::String(diff) + ")");
            expect(isBufferFinite(bufNarrow), "PW output should be finite");
        }

        // ── HARD SYNC CHANGES OUTPUT ───────────────────────────────
        beginTest("Hard sync creates harmonic change");
        {
            SynthConfig noSync;
            noSync.sync = 0.f;

            SynthConfig withSync;
            withSync.sync = 0.6f;

            auto bufClean = renderWithConfig(noSync, sawMip, sawMip);
            auto bufSync  = renderWithConfig(withSync, sawMip, sawMip);

            float diff = bufferDiff(bufClean, bufSync);
            expect(diff > 0.01f, "Sync should change output (diff=" + juce::String(diff) + ")");
            expect(isBufferFinite(bufSync), "Sync output should be finite");
        }

        // ── METAL FORGE CONFIG: 8 unison + fold 0.8 + ring mod ────
        beginTest("Metal Forge config: 8 unison + fold + ring (stability)");
        {
            SynthConfig cfg;
            cfg.unisonVoices = 8;
            cfg.detune = 25.f;
            cfg.spread = 100.f;
            cfg.blend = 0.45f;
            cfg.fold = 0.8f;
            cfg.oscBEnable = true;
            cfg.oscBLevel = 0.3f;
            cfg.oscBSemi = 7;
            cfg.oscBMixMode = 1; // Ring
            cfg.attack = 0.001f;
            cfg.decay = 0.6f;
            cfg.sustain = 0.4f;
            cfg.release = 1.5f;
            cfg.numSamples = 4096;

            auto buf = renderWithConfig(cfg, sawMip, sawMip);

            expect(isBufferFinite(buf), "Metal Forge config should produce finite output");
            expect(buf.getRMSLevel(0, 0, 4096) > 0.001f, "Metal Forge should produce audible output");
            // Check peak isn't insanely loud (wavefolding + 8 unison could explode)
            float peak = buf.getMagnitude(0, 4096);
            expect(peak < 10.f, "Metal Forge peak should be bounded (peak=" + juce::String(peak) + ")");
        }

        // ── FROZEN TUNDRA CONFIG: 7 unison + PD + Osc B AM ────────
        beginTest("Frozen Tundra config: 7 unison + PD + AM (stability)");
        {
            SynthConfig cfg;
            cfg.unisonVoices = 7;
            cfg.detune = 10.f;
            cfg.spread = 95.f;
            cfg.blend = 0.5f;
            cfg.pd = 0.15f;
            cfg.oscBEnable = true;
            cfg.oscBLevel = 0.2f;
            cfg.oscBSemi = 19;
            cfg.oscBMixMode = 3; // AM
            cfg.attack = 1.5f;
            cfg.decay = 0.6f;
            cfg.sustain = 0.85f;
            cfg.release = 4.0f;
            cfg.numSamples = 4096;

            auto buf = renderWithConfig(cfg, sineMip, sineMip);

            expect(isBufferFinite(buf), "Frozen Tundra config should produce finite output");
            // Slow attack means RMS might be low in first 4096 samples, that's OK
            // Just verify no NaN/Inf and some output
            float rmsL = buf.getRMSLevel(0, 0, 4096);
            float rmsR = buf.getRMSLevel(1, 0, 4096);
            expect(rmsL > 0.0001f || rmsR > 0.0001f,
                   "Frozen Tundra should produce some output (rmsL=" + juce::String(rmsL) + ")");
        }

        // ── ALIEN ARTIFACT CONFIG: PD + FM + 4 unison ──────────────
        beginTest("Alien Artifact config: PD + FM + 4 unison");
        {
            SynthConfig cfg;
            cfg.unisonVoices = 4;
            cfg.detune = 18.f;
            cfg.spread = 70.f;
            cfg.blend = 0.4f;
            cfg.pd = 0.4f;
            cfg.oscBEnable = true;
            cfg.oscBLevel = 0.4f;
            cfg.oscBSemi = 7;
            cfg.oscBFine = 3.f;
            cfg.oscBMixMode = 2; // FM
            cfg.oscBFMDepth = 0.3f;
            cfg.attack = 0.3f;
            cfg.decay = 0.5f;
            cfg.sustain = 0.8f;
            cfg.release = 1.5f;
            cfg.numSamples = 4096;

            auto buf = renderWithConfig(cfg, sawMip, sawMip);

            expect(isBufferFinite(buf), "Alien Artifact config should produce finite output");
            expect(buf.getRMSLevel(0, 0, 4096) > 0.0001f, "Alien Artifact should produce output");
        }

        // ── LAVA FLOW CONFIG: fold + ring + noise path ─────────────
        beginTest("Lava Flow config: fold + ring mod (stability)");
        {
            SynthConfig cfg;
            cfg.unisonVoices = 2;
            cfg.detune = 5.f;
            cfg.spread = 60.f;
            cfg.blend = 0.6f;
            cfg.fold = 0.7f;
            cfg.oscBEnable = true;
            cfg.oscBLevel = 0.3f;
            cfg.oscBSemi = 7;
            cfg.oscBMixMode = 1; // Ring
            cfg.attack = 0.01f;
            cfg.decay = 0.15f;
            cfg.sustain = 0.85f;
            cfg.release = 0.3f;
            cfg.numSamples = 4096;

            auto buf = renderWithConfig(cfg, sawMip, sawMip);

            expect(isBufferFinite(buf), "Lava Flow config should produce finite output");
            expect(buf.getRMSLevel(0, 0, 4096) > 0.001f, "Lava Flow should produce audible output");
        }

        // ── CIRCUIT PULSE CONFIG: PW + sync, no unison ─────────────
        beginTest("Circuit Pulse config: PW + sync mono");
        {
            SynthConfig cfg;
            cfg.unisonVoices = 1;
            cfg.pw = 0.2f;
            cfg.sync = 0.6f;
            cfg.attack = 0.001f;
            cfg.decay = 0.08f;
            cfg.sustain = 0.9f;
            cfg.release = 0.1f;
            cfg.numSamples = 2048;

            auto buf = renderWithConfig(cfg, sawMip, sawMip);

            expect(isBufferFinite(buf), "Circuit Pulse config should produce finite output");
            expect(buf.getRMSLevel(0, 0, 2048) > 0.001f, "Circuit Pulse should produce output");
        }

        // ── SCALES OF LEVIATHAN: sub-octave Osc B + fold ───────────
        beginTest("Scales of Leviathan config: sub-octave Osc B");
        {
            SynthConfig cfg;
            cfg.unisonVoices = 4;
            cfg.detune = 8.f;
            cfg.spread = 40.f;
            cfg.blend = 0.65f;
            cfg.fold = 0.2f;
            cfg.oscBEnable = true;
            cfg.oscBLevel = 0.6f;
            cfg.oscBSemi = -12;  // sub octave
            cfg.oscBMixMode = 0; // Add
            cfg.attack = 0.05f;
            cfg.decay = 0.2f;
            cfg.sustain = 0.85f;
            cfg.release = 0.6f;
            cfg.numSamples = 4096;

            auto buf = renderWithConfig(cfg, sawMip, sawMip);

            expect(isBufferFinite(buf), "Leviathan config should produce finite output");
            expect(buf.getRMSLevel(0, 0, 4096) > 0.001f, "Leviathan should produce output");
        }

        // ── MULTI-NOTE POLYPHONY: 4 simultaneous notes ─────────────
        beginTest("4-voice polyphony with Phase 2 features (no NaN)");
        {
            SDFSynthesiser synth;
            synth.setCurrentPlaybackSampleRate(44100.0);
            synth.setMipMappedWavetable(sawMip);
            synth.setMipMappedWavetableB(sineMip);

            synth.updateUnisonParams(3, 12.f, 60.f, 0.5f);
            synth.updateOscEffects(0.4f, 0.2f, 0.5f, 0.f);
            synth.updateOscBParams(true, 0.35f, 7, 0.f, 1, 0.f); // Ring

            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 48, (juce::uint8)90), 0);
            midi.addEvent(juce::MidiMessage::noteOn(1, 55, (juce::uint8)85), 0);
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            midi.addEvent(juce::MidiMessage::noteOn(1, 67, (juce::uint8)80), 0);

            juce::AudioBuffer<float> buf(2, 4096);
            buf.clear();
            synth.renderNextBlock(buf, midi, 0, 4096);

            expect(isBufferFinite(buf), "4-voice polyphony should produce finite output");
            expect(buf.getRMSLevel(0, 0, 4096) > 0.001f, "Polyphonic output should be audible");
            expect(synth.getActiveVoiceCount() >= 4,
                   "Should have at least 4 active voices (got "
                   + juce::String(synth.getActiveVoiceCount()) + ")");
        }

        // ── NOTE OFF + RELEASE: voices release cleanly ──────────────
        beginTest("Note-off with Phase 2 features releases cleanly");
        {
            SDFSynthesiser synth;
            synth.setCurrentPlaybackSampleRate(44100.0);
            synth.setMipMappedWavetable(sineMip);
            synth.setMipMappedWavetableB(sineMip);

            synth.updateUnisonParams(4, 15.f, 80.f, 0.6f);
            synth.updateOscEffects(0.3f, 0.f, 0.5f, 0.f);
            synth.updateOscBParams(true, 0.4f, 12, 0.f, 0, 0.f); // Add, octave up
            synth.updateADSR(0.01f, 0.1f, 0.8f, 0.5f);

            // Note on
            juce::MidiBuffer midi1;
            midi1.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            juce::AudioBuffer<float> buf1(2, 2048);
            buf1.clear();
            synth.renderNextBlock(buf1, midi1, 0, 2048);

            float rmsBeforeOff = buf1.getRMSLevel(0, 1024, 1024);

            // Note off
            juce::MidiBuffer midi2;
            midi2.addEvent(juce::MidiMessage::noteOff(1, 60, (juce::uint8)0), 0);
            juce::AudioBuffer<float> buf2(2, 8192);
            buf2.clear();
            synth.renderNextBlock(buf2, midi2, 0, 8192);

            // After release, tail should die out
            float rmsEnd = buf2.getRMSLevel(0, 7000, 1192);
            expect(isBufferFinite(buf2), "Release tail should be finite");
            expect(rmsEnd < rmsBeforeOff,
                   "Release tail RMS should be lower than sustain (end="
                   + juce::String(rmsEnd) + " sustain=" + juce::String(rmsBeforeOff) + ")");
        }

        // ── EXTREME DETUNE: 100ct with 8 unison ────────────────────
        beginTest("Extreme detune 100ct with 8 unison");
        {
            SynthConfig cfg;
            cfg.unisonVoices = 8;
            cfg.detune = 100.f;  // max detune
            cfg.spread = 100.f;
            cfg.blend = 0.5f;
            cfg.numSamples = 4096;

            auto buf = renderWithConfig(cfg, sawMip, sawMip);

            expect(isBufferFinite(buf), "Extreme detune should produce finite output");
            expect(buf.getRMSLevel(0, 0, 4096) > 0.001f, "Extreme detune should produce output");
        }

        // ── NOISE GENERATOR + FILTER PIPELINE ──────────────────────
        beginTest("Noise through SVF filter produces valid output");
        {
            NoiseGenerator noise;
            noise.reset();
            noise.setSampleRate(44100.f);

            SVFFilter lpf;
            lpf.reset();
            lpf.setParams(800.f, 0.15f, 44100.f);
            lpf.setMode(FilterMode::LowPass);

            bool valid = true;
            double sumSq = 0.0;
            constexpr int N = 10000;

            for (int i = 0; i < N; ++i)
            {
                float n = noise.nextSample(NoiseType::Brown);
                float out = lpf.process(n * 0.06f); // scaled like preset noise level
                if (std::isnan(out) || std::isinf(out))
                {
                    valid = false;
                    break;
                }
                sumSq += static_cast<double>(out) * static_cast<double>(out);
            }
            float rms = static_cast<float>(std::sqrt(sumSq / N));
            expect(valid, "Noise through filter should produce finite output");
            expect(rms > 0.0001f, "Filtered noise should have some energy (rms=" + juce::String(rms) + ")");
            expect(rms < 1.f, "Filtered noise should be reasonably bounded");
        }

        // ── HIGH NOTE + SUB-OCTAVE OSC B ───────────────────────────
        beginTest("High MIDI note (C7=96) with sub-octave Osc B");
        {
            SynthConfig cfg;
            cfg.midiNote = 96;  // C7 ~2093 Hz
            cfg.oscBEnable = true;
            cfg.oscBSemi = -12; // sub octave → ~1047 Hz
            cfg.oscBLevel = 0.5f;
            cfg.oscBMixMode = 0; // Add
            cfg.numSamples = 2048;

            auto buf = renderWithConfig(cfg, sineMip, sineMip);
            expect(isBufferFinite(buf), "High note + sub Osc B should be finite");
            expect(buf.getRMSLevel(0, 0, 2048) > 0.001f, "Should produce output at C7");
        }

        beginTest("Low MIDI note (C1=24) with octave-up Osc B");
        {
            SynthConfig cfg;
            cfg.midiNote = 24;  // C1 ~32.7 Hz
            cfg.oscBEnable = true;
            cfg.oscBSemi = 12;
            cfg.oscBLevel = 0.5f;
            cfg.oscBMixMode = 0;
            cfg.numSamples = 4096;

            auto buf = renderWithConfig(cfg, sineMip, sineMip);
            expect(isBufferFinite(buf), "Low note + octave-up Osc B should be finite");
            expect(buf.getRMSLevel(0, 0, 4096) > 0.0001f, "Should produce output at C1");
        }
    }
};

static Phase2IntegrationTest phase2IntegrationTest;
