#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "GUI/SDFLookAndFeel.h"
#include "GUI/ArcKnob.h"
#include "GUI/ADSRDisplay.h"
#include "GUI/ShapeSelector.h"
#include "GUI/WaveformScope.h"
#include "GUI/SDFViewport3D.h"
#include "GUI/TextureMenu.h"
#include "Utility/PresetManager.h"
#include "Texture/ProceduralLibrary.h"

class SDFSynthEditor : public juce::AudioProcessorEditor,
                        public juce::Timer,
                        public juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit SDFSynthEditor(SDFSynthProcessor&);
    ~SDFSynthEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    SDFSynthProcessor& processor;
    SDFLookAndFeel lookAndFeel;
    PresetManager presetManager;
    juce::ComboBox presetSelector;
    juce::TextButton presetSaveBtn{ "SAVE" }, presetLoadBtn{ "LOAD" };

    // 3D Viewport
    SDFViewport3D viewport3D;

    // Texture menu (must be after viewport3D for GL context access)
    TextureMenu textureMenu;

    // Shape selectors
    ShapeSelector shapeASelector;
    ShapeSelector shapeBSelector;

    // Operation buttons
    juce::OwnedArray<juce::TextButton> opButtons;

    // Scan mode buttons
    juce::OwnedArray<juce::TextButton> scanModeButtons;

    // Knobs - Scene
    ArcKnob sizeAKnob, sizeBKnob, offsetXKnob, offsetYKnob;
    ArcKnob smoothKKnob, twistKnob;

    // Knobs - Scan
    ArcKnob scanRadiusKnob, scanHeightKnob, topoMorphKnob;

    // Knobs - Scale/Audio
    ArcKnob distScaleKnob;
    ArcKnob filterCutKnob, filterResKnob;
    ArcKnob attackKnob, decayKnob, sustainKnob, releaseKnob;
    ArcKnob gainKnob;

    // Graphic ADSR display (alongside ADSR knobs)
    ADSRDisplay adsrDisplay;

    // Filter mode selector
    juce::ComboBox filterModeSelector;

    // Waveform
    WaveformScope waveformScope;

    // Voice activity meter
    int lastVoiceCount = 0;

    // Skybox controls
    juce::ComboBox skyboxSelector;
    ArcKnob skyExpKnob, skyRotKnob, skyReflKnob, skyBlurKnob;
    juce::TextButton skyLoadBtn{ "HDR" };

    // Collapsible MIDI keyboard
    juce::MidiKeyboardComponent midiKeyboard;
    juce::TextButton keyboardToggle{ "KB" };
    bool keyboardVisible = false;
    static constexpr int kKeyboardHeight = 72;

    // Panel bounds (computed in resized(), drawn in paint())
    juce::Rectangle<int> shapePanelBounds;
    juce::Rectangle<int> scenePanelBounds;
    juce::Rectangle<int> scanPanelBounds;
    juce::Rectangle<int> filterEnvPanelBounds;
    juce::Rectangle<int> visualPanelBounds;
    juce::Rectangle<int> voiceMeterBounds;

    void setupOperationButtons();
    void setupScanModeButtons();
    void setupSkyboxSelector();
    void setupFilterModeSelector();
    void applyPresetResources();
    void updateScanKnobLabels(int mode);
    void setupModRouting();
    void updateModDepthDisplays();
    ArcKnob* findKnobAt(juce::Point<int> pos);

    // Mod routing drag state
    std::vector<ArcKnob*> modTargetKnobs;
    ArcKnob* currentModHighlight = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SDFSynthEditor)
};
