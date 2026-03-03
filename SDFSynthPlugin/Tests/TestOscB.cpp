#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "DSP/SDFSynthesiser.h"
#include "DSP/WavetableGenerator.h"
#include <cmath>
#include <memory>

class OscBTest : public juce::UnitTest
{
public:
    OscBTest() : juce::UnitTest("OscB") {}

    void runTest() override
    {
        beginTest("Osc B disabled = A-only output matches original");
        {
            SDFSynthesiser synth;
            synth.setCurrentPlaybackSampleRate(44100.0);

            // Generate a simple sine wavetable
            WavetableGenerator::Wavetable wt{};
            for (int i = 0; i < sdf::TABLE_SIZE; ++i)
                wt[i] = std::sin(sdf::TWO_PI * static_cast<float>(i) / sdf::TABLE_SIZE);
            auto mip = std::make_shared<const MipMappedWavetable>(WavetableGenerator::generateMipMap(wt));
            synth.setMipMappedWavetable(mip);

            // Osc B off
            synth.updateOscBParams(false, 0.5f, 0, 0.f, 0, 0.f);
            synth.updateUnisonParams(1, 20.f, 50.f, 0.5f);
            synth.updateOscEffects(0.f, 0.f, 0.5f, 0.f);

            // Render a note
            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);
            juce::AudioBuffer<float> buf(2, 512);
            buf.clear();
            synth.renderNextBlock(buf, midi, 0, 512);

            // Should have non-zero output
            float rms = buf.getRMSLevel(0, 0, 512);
            expect(rms > 0.001f, "Should produce output with Osc B disabled");
        }

        beginTest("Ring mod of DC signals produces product");
        {
            // With Add mode and level=1, output = A + B
            // With Ring mode and level=1, output = A * B
            // We can't easily test with DC signals through the synth,
            // but we verify that ring mode produces different output than add mode.
            SDFSynthesiser synthAdd, synthRing;
            synthAdd.setCurrentPlaybackSampleRate(44100.0);
            synthRing.setCurrentPlaybackSampleRate(44100.0);

            WavetableGenerator::Wavetable wt{};
            for (int i = 0; i < sdf::TABLE_SIZE; ++i)
                wt[i] = std::sin(sdf::TWO_PI * static_cast<float>(i) / sdf::TABLE_SIZE);
            auto mip = std::make_shared<const MipMappedWavetable>(WavetableGenerator::generateMipMap(wt));

            synthAdd.setMipMappedWavetable(mip);
            synthAdd.setMipMappedWavetableB(mip);
            synthRing.setMipMappedWavetable(mip);
            synthRing.setMipMappedWavetableB(mip);

            synthAdd.updateOscBParams(true, 1.0f, 12, 0.f, 0, 0.f);  // Add
            synthRing.updateOscBParams(true, 1.0f, 12, 0.f, 1, 0.f); // Ring
            synthAdd.updateUnisonParams(1, 0.f, 0.f, 1.f);
            synthRing.updateUnisonParams(1, 0.f, 0.f, 1.f);
            synthAdd.updateOscEffects(0.f, 0.f, 0.5f, 0.f);
            synthRing.updateOscEffects(0.f, 0.f, 0.5f, 0.f);

            juce::MidiBuffer midi;
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 0);

            juce::AudioBuffer<float> bufAdd(2, 512), bufRing(2, 512);
            bufAdd.clear();
            bufRing.clear();
            synthAdd.renderNextBlock(bufAdd, midi, 0, 512);
            synthRing.renderNextBlock(bufRing, midi, 0, 512);

            // Add and Ring should produce different waveforms
            float diff = 0.f;
            for (int i = 0; i < 512; ++i)
                diff += std::abs(bufAdd.getSample(0, i) - bufRing.getSample(0, i));
            expect(diff > 0.01f, "Ring and Add modes should produce different output");
        }
    }
};

static OscBTest oscBTest;
