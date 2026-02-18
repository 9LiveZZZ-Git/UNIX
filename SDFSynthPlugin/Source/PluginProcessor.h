#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/SDFScene3D.h"
#include "DSP/ContourExtractor.h"
#include "DSP/WavetableGenerator.h"
#include "DSP/SDFSynthesiser.h"
#include "DSP/SVFFilter.h"
#include "DSP/ScanMode.h"
#include "Texture/TextureSystem.h"
#include "Utility/Constants.h"
#include "Utility/ThreadSafeSwap.h"
#include <thread>
#include <atomic>

class SDFSynthProcessor : public juce::AudioProcessor,
                           public juce::AudioProcessorValueTreeState::Listener
{
public:
    SDFSynthProcessor();
    ~SDFSynthProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;

    // Expose for GUI
    const WavetableGenerator::Wavetable& getCurrentWavetable() const { return currentWavetable; }
    const std::vector<ContourPoint>& getCurrentContour() const { return currentContour; }
    float getActivePhase() const { return synthesiser.getActivePhase(); }
    int getActiveVoiceCount() const { return synthesiser.getActiveVoiceCount(); }

    // Texture system reference (set by editor, used for audio modulation)
    void setTextureSystem(TextureSystem* ts) { texSystem = ts; }
    void markWavetableDirty() { wavetableDirty.store(true); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void rebuildWavetable();
    void triggerBackgroundRebuild();
    void updateSceneFromParams();
    void handleMidiCC(int controller, int value);

    SDFScene3D scene;
    ContourExtractor contourExtractor;
    WavetableGenerator::Wavetable currentWavetable{};
    WavetableGenerator::Wavetable activeWavetable{}; // what the audio thread reads
    std::vector<ContourPoint> currentContour;
    SDFSynthesiser synthesiser;

    std::atomic<bool> wavetableDirty{ true };
    TextureSystem* texSystem = nullptr;

    // Filter
    SVFFilter filterL, filterR;
    float lastFilterCutoff = -1.f, lastFilterRes = -1.f;

    // Crossfade length (samples) for oscillator-level table blending
    static constexpr int CROSSFADE_SAMPLES = 64;

    // Background wavetable rebuild
    std::thread builderThread;
    std::atomic<bool> rebuildInProgress{ false };
    std::atomic<bool> newTableReady{ false };

    // Pending results from background thread
    MipMappedWavetable pendingMipTable{};
    WavetableGenerator::Wavetable pendingWavetable{};
    std::vector<ContourPoint> pendingContour;

    // ADAA tanh state (per channel)
    float adaaPrevX[2] = { 0.f, 0.f };
    float adaaPrevF[2] = { 0.f, 0.f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SDFSynthProcessor)
};
