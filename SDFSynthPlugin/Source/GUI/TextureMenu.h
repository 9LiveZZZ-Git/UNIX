#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Texture/TextureSystem.h"
#include "Texture/ProceduralLibrary.h"
#include "GUI/ArcKnob.h"
#include "GUI/SDFLookAndFeel.h"

class TextureMenu : public juce::Component
{
public:
    TextureMenu(juce::AudioProcessorValueTreeState& apvts,
                TextureSystem& texSystem);
    ~TextureMenu() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Call when a procedural preset is loaded (triggers wavetable dirty flag externally)
    std::function<void()> onTextureChanged;

private:
    juce::AudioProcessorValueTreeState& apvts;
    TextureSystem& textureSystem;

    juce::ComboBox presetSelector;

    // Upload buttons for individual texture channels
    juce::TextButton colorBtn{ "COL" }, normalBtn{ "NRM" }, roughBtn{ "RGH" },
                     dispBtn{ "DSP" }, aoBtn{ "AO" }, emitBtn{ "EMT" };
    juce::TextButton clearBtn{ "CLR" };
    juce::TextButton matBtn{ "MAT" }; // Load full PBR material set from folder

    // Texture parameter knobs
    ArcKnob texScaleKnob, texBrightKnob, texBlendKnob, normIntKnob;
    ArcKnob roughOffKnob, dispAmtKnob, aoIntKnob, emIntKnob;

    void loadPreset(int presetIndex);
    void browseForTexture(TextureSlot& slot);
    void browseForMaterial();

    // External PBR material presets (e.g. pine forest pack)
    struct ExternalPreset
    {
        juce::String displayName;
        juce::File   directory;
        juce::String baseName;
    };
    std::vector<ExternalPreset> externalPresets;
    int externalIdOffset = 0; // ComboBox ID where externals start

    void scanExternalPresets(const juce::File& dir);
    void loadExternalPreset(int externalIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextureMenu)
};
