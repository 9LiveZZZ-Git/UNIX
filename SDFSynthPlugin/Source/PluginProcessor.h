#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SDFScene3D.h"
#include "DSP/ContourExtractor.h"
#include "DSP/WavetableGenerator.h"
#include "DSP/SDFSynthesiser.h"
#include "DSP/SVFFilter.h"
#include "DSP/NoiseGenerator.h"
#include "DSP/LFO.h"
#include "DSP/ModulationMatrix.h"
#include "DSP/FXChain.h"
#include "DSP/ScanMode.h"
#include "DSP/VoxelSDF.h"
#include "DSP/MeshImporter.h"
#include "Texture/TextureSystem.h"
#include "Utility/Constants.h"
#include "Utility/ThreadSafeSwap.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

class SDFSynthProcessor : public juce::AudioProcessor,
                           public juce::AudioProcessorValueTreeState::Listener,
                           private juce::Timer
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
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;

    // Keyboard state — shared with editor's MidiKeyboardComponent
    juce::MidiKeyboardState keyboardState;

    // UI state (persisted but not APVTS)
    int savedWindowWidth = 950;
    int savedWindowHeight = 760;
    float savedUiScale = 1.0f;

    // Debug bypass flags (not persisted, for isolating audio issues)
    std::atomic<bool> bypassOsc { false };  // skip osc effects, osc B, unison, noise
    std::atomic<bool> bypassFX  { false };  // skip filter + FX chain

    // Expose for GUI
    const WavetableGenerator::Wavetable& getCurrentWavetable() const { return currentWavetable; }
    std::shared_ptr<const std::vector<ContourPoint>> getCurrentContourPtr() const { return currentContour; }
    const std::vector<ContourPoint>& getCurrentContour() const
    {
        static const std::vector<ContourPoint> empty;
        return currentContour ? *currentContour : empty;
    }
    float getActivePhase() const { return synthesiser.getActivePhase(); }
    int getActiveVoiceCount() const { return synthesiser.getActiveVoiceCount(); }

    // Texture system reference (set by editor, used for audio modulation)
    void setTextureSystem(TextureSystem* ts) { texSystem = ts; }
    void markWavetableDirty() { wavetableDirty.store(true); }

    // OBJ mesh import
    void importOBJFile(const juce::File& file);
    bool isVoxelizing() const { return voxelizing.load(std::memory_order_relaxed); }
    juce::String getCustomMeshName() const;
    std::shared_ptr<const VoxelSDF> getCurrentVoxelSDF() const;

    // Modulation matrix (new Phase 3 system)
    ModulationMatrix& getModMatrix() { return modMatrix; }
    const ModulationMatrix& getModMatrix() const { return modMatrix; }

    float getLastEnvelopeValue() const { return lastEnvValue.load(); }
    float getLFOValue(int lfoIndex) const
    {
        if (lfoIndex == 0) return lfo1Value.load(std::memory_order_relaxed);
        if (lfoIndex == 1) return lfo2Value.load(std::memory_order_relaxed);
        return 0.f;
    }

    float getModulatedParamValue(const juce::String& paramId) const;
    static const std::vector<juce::String>& getModulatableParamIds()
    {
        static const std::vector<juce::String> ids = {
            "size1", "size2", "offsetX", "offsetY", "smoothK", "twist",
            "scanRadius", "scanHeight", "topoMorph", "distScale",
            "filterCutoff", "filterRes",
            "attack", "decay", "sustain", "release",
            "masterGain",
            "oscFold", "oscPhaseDist", "oscPW", "oscSync",
            "unisonDetune", "unisonSpread",
            "oscBLevel", "oscBFMDepth",
            "noiseLevel", "noiseFilterCutoff",
            "fxDistDrive", "fxChorusRate", "fxDelayFeedback", "fxReverbSize"
        };
        return ids;
    }

    // Dest index lookup from param ID
    static int getDestIndex(const juce::String& paramId)
    {
        const auto& ids = getModulatableParamIds();
        for (int i = 0; i < static_cast<int>(ids.size()); ++i)
            if (ids[static_cast<size_t>(i)] == paramId) return i;
        return -1;
    }

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
    std::shared_ptr<const std::vector<ContourPoint>> currentContour;
    SDFSynthesiser synthesiser;

    std::atomic<bool> wavetableDirty{ true };
    TextureSystem* texSystem = nullptr;

    // Filter
    SVFFilter filterL, filterR;
    float lastFilterCutoff = -1.f, lastFilterRes = -1.f;

    // Crossfade length (samples) for oscillator-level table blending
    static constexpr int CROSSFADE_SAMPLES = 512;

    void timerCallback() override;

    // Background wavetable rebuild
    std::thread builderThread;
    std::atomic<bool> rebuildInProgress{ false };
    std::atomic<bool> newTableReady{ false };

    // Pending results from background thread (shared_ptr: zero-copy handoff to audio thread)
    std::shared_ptr<const MipMappedWavetable> pendingMipTable;
    WavetableGenerator::Wavetable pendingWavetable{};
    std::shared_ptr<const std::vector<ContourPoint>> pendingContour;

    // Noise generator + dedicated LP filter (stereo)
    NoiseGenerator noiseGen;
    SVFFilter noiseFilterL, noiseFilterR;

    // Osc B background table
    std::shared_ptr<const MipMappedWavetable> pendingMipTableB;
    std::atomic<bool> newTableBReady{ false };

    // Oversampling (pre-created in prepareToPlay)
    std::unique_ptr<juce::dsp::Oversampling<float>> os2x;
    std::unique_ptr<juce::dsp::Oversampling<float>> os4x;
    juce::AudioBuffer<float> osScratchBuffer;  // pre-allocated for oversampling
    double baseSampleRate = 44100.0;

    // Smoothed master gain (20ms ramp to prevent clicks)
    juce::SmoothedValue<float> smoothedGain;

    // ADAA tanh state (per channel)
    float adaaPrevX[2] = { 0.f, 0.f };
    float adaaPrevF[2] = { 0.f, 0.f };

    // Cached APVTS parameter values — snapshot once per processBlock
    struct ParamCache
    {
        // LFO
        float lfo1Rate, lfo1Phase, lfo2Rate, lfo2Phase;
        int lfo1Shape, lfo2Shape;
        bool lfo1Sync, lfo2Sync;

        // Macros
        float macro1, macro2, macro3, macro4;

        // Oversampling
        int oversample;

        // Noise
        bool noiseEnable;
        float noiseLevel;
        int noiseType;
        float noiseFilterCutoff;

        // Osc B
        bool oscBEnable;
        int oscBSemitone, oscBMixMode;
        float oscBFine;

        // Unison
        int unisonVoices;
        float unisonBlend;

        // Filter
        int filterMode;

        // Master
        float masterGain;

        // FX enables
        bool fxDistEnable, fxChorusEnable, fxDelayEnable, fxReverbEnable;

        // FX non-modulatable
        float fxDistMix, fxChorusDepth, fxChorusMix;
        float fxDelayTime, fxDelayMix;
        float fxReverbDamping, fxReverbMix;
        int fxDistType;
    };

    ParamCache paramCache{};
    void snapshotParams();

    // LFO engines
    LFO lfo1, lfo2;
    std::atomic<float> lfo1Value{ 0.f };
    std::atomic<float> lfo2Value{ 0.f };

    // Modulation matrix (replaces old ModRoute/mutex)
    ModulationMatrix modMatrix;
    std::atomic<float> lastEnvValue{ 0.f };
    std::atomic<float> lastModWheel{ 0.f };
    std::atomic<float> lastAftertouch{ 0.f };

    // FX chain (post-filter)
    FXChain fxChain;

    // Destruction flag to prevent background rebuilds during teardown
    std::atomic<bool> destroying{ false };

    // Voxel SDF (OBJ mesh import)
    ThreadSafeSwap<const VoxelSDF> voxelSwap;
    std::shared_ptr<const VoxelSDF> currentVoxelSDF;
    std::thread voxelThread;
    std::atomic<bool> voxelizing{ false };
    juce::String customOBJPath;
    mutable std::mutex objPathMutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SDFSynthProcessor)
};
