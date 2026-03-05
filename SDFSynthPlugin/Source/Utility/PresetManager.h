#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

enum class PresetCategory { Pad, Lead, Bass, Key, FX, Perc };

class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    void savePreset(const juce::File& file);
    bool loadPreset(const juce::File& file);

    static juce::StringArray getFactoryPresetNames();
    void loadFactoryPreset(int index);

    juce::File getPresetsFolder() const;

    // Texture/skybox key from last loaded factory preset (empty = none)
    juce::String getLastTextureKey() const { return lastTextureKey; }
    juce::String getLastSkyboxKey() const { return lastSkyboxKey; }
    int getLastSkyboxIndex() const { return lastSkyboxIndex; }

    // Init: reset all params to defaults
    void loadInitPreset();

    // A/B snapshot comparison
    void snapshotA();
    void snapshotB();
    void loadA();
    void loadB();
    bool isOnB() const { return currentIsB; }

    // Category helpers
    static juce::String getCategoryName(PresetCategory cat);
    static std::vector<PresetCategory> getAllCategories();

    struct FactoryPreset
    {
        juce::String name;
        std::vector<std::pair<juce::String, float>> params;
        juce::String textureKey;   // procedural library key (empty = none)
        int skyboxPreset = -1;     // -1 = no change, 0-5 = preset index
        PresetCategory category = PresetCategory::Pad;
    };

    static std::vector<FactoryPreset> getFactoryPresets();

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::String lastTextureKey;
    juce::String lastSkyboxKey;
    int lastSkyboxIndex = -1;

    // A/B snapshot state
    juce::ValueTree snapshotStateA, snapshotStateB;
    juce::String snapshotTexA, snapshotTexB;
    int snapshotSkyA = -1, snapshotSkyB = -1;
    bool currentIsB = false;
};
