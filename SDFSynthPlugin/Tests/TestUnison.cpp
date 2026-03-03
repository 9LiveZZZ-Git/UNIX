#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "DSP/SDFSynthesiser.h"
#include "DSP/WavetableGenerator.h"
#include <cmath>
#include <memory>

class UnisonTest : public juce::UnitTest
{
public:
    UnisonTest() : juce::UnitTest("Unison") {}

    void runTest() override
    {
        beginTest("N=1 produces output");
        {
            SDFSynthesiser synth;
            synth.setCurrentPlaybackSampleRate(44100.0);

            WavetableGenerator::Wavetable wt{};
            for (int i = 0; i < sdf::TABLE_SIZE; ++i)
                wt[i] = std::sin(sdf::TWO_PI * static_cast<float>(i) / sdf::TABLE_SIZE);
            auto mip = std::make_shared<const MipMappedWavetable>(WavetableGenerator::generateMipMap(wt));
            synth.setMipMappedWavetable(mip);

            synth.updateUnisonParams(1, 20.f, 50.f, 0.5f);
            synth.updateOscEffects(0.f, 0.f, 0.5f, 0.f);
            synth.updateOscBParams(false, 0.5f, 0, 0.f, 0, 0.f);

            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            juce::AudioBuffer<float> buf(2, 512);
            buf.clear();
            synth.renderNextBlock(buf, midi, 0, 512);

            float rms = buf.getRMSLevel(0, 0, 512);
            expect(rms > 0.001f, "N=1 should produce output");
        }

        beginTest("N=2 produces stereo spread");
        {
            SDFSynthesiser synth;
            synth.setCurrentPlaybackSampleRate(44100.0);

            WavetableGenerator::Wavetable wt{};
            for (int i = 0; i < sdf::TABLE_SIZE; ++i)
                wt[i] = std::sin(sdf::TWO_PI * static_cast<float>(i) / sdf::TABLE_SIZE);
            auto mip = std::make_shared<const MipMappedWavetable>(WavetableGenerator::generateMipMap(wt));
            synth.setMipMappedWavetable(mip);

            synth.updateUnisonParams(2, 20.f, 100.f, 1.0f); // full spread
            synth.updateOscEffects(0.f, 0.f, 0.5f, 0.f);
            synth.updateOscBParams(false, 0.5f, 0, 0.f, 0, 0.f);

            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            juce::AudioBuffer<float> buf(2, 512);
            buf.clear();
            synth.renderNextBlock(buf, midi, 0, 512);

            // With full spread and detune, L and R channels should differ
            float diffSum = 0.f;
            for (int i = 0; i < 512; ++i)
                diffSum += std::abs(buf.getSample(0, i) - buf.getSample(1, i));

            expect(diffSum > 0.01f, "N=2 with 100% spread should have stereo difference");
        }

        beginTest("N=8 doesn't crash or produce NaN");
        {
            SDFSynthesiser synth;
            synth.setCurrentPlaybackSampleRate(44100.0);

            WavetableGenerator::Wavetable wt{};
            for (int i = 0; i < sdf::TABLE_SIZE; ++i)
                wt[i] = std::sin(sdf::TWO_PI * static_cast<float>(i) / sdf::TABLE_SIZE);
            auto mip = std::make_shared<const MipMappedWavetable>(WavetableGenerator::generateMipMap(wt));
            synth.setMipMappedWavetable(mip);

            synth.updateUnisonParams(8, 50.f, 80.f, 0.5f);
            synth.updateOscEffects(0.f, 0.f, 0.5f, 0.f);
            synth.updateOscBParams(false, 0.5f, 0, 0.f, 0, 0.f);

            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            juce::AudioBuffer<float> buf(2, 1024);
            buf.clear();
            synth.renderNextBlock(buf, midi, 0, 1024);

            bool valid = true;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 1024; ++i)
                    if (std::isnan(buf.getSample(ch, i)) || std::isinf(buf.getSample(ch, i)))
                        valid = false;

            expect(valid, "N=8 output should be all finite");
            expect(buf.getRMSLevel(0, 0, 1024) > 0.001f, "N=8 should produce output");
        }
    }
};

static UnisonTest unisonTest;
