#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "GUI/SDFLookAndFeel.h"
#include "GUI/ArcKnob.h"
#include "GUI/WaveformScope.h"
#include "GUI/SDFViewport3D.h"
#include "GUI/TabBar.h"
#include "GUI/BottomStrip.h"
#include "GUI/ShapeTab.h"
#include "GUI/OscTab.h"
#include "GUI/FXTab.h"
#include "GUI/ModTab.h"
#include "GUI/VisTab.h"
#include "GUI/ScanModeOverlay.h"
#include "Utility/PresetManager.h"
#include "Texture/ProceduralLibrary.h"

class SDFSynthEditor : public juce::AudioProcessorEditor,
                        public juce::Timer,
                        public juce::AudioProcessorValueTreeState::Listener,
                        public juce::FileDragAndDropTarget
{
public:
    explicit SDFSynthEditor(SDFSynthProcessor&);
    ~SDFSynthEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    SDFSynthProcessor& processor;
    SDFLookAndFeel lookAndFeel;
    PresetManager presetManager;

    // Header bar
    juce::TextButton presetBrowserBtn{ "-- Preset --" };
    juce::TextButton initBtn{ "INIT" };
    juce::TextButton abBtn{ "A" };
    juce::TextButton presetSaveBtn{ "SAVE" }, presetLoadBtn{ "LOAD" };
    juce::ComboBox scaleSelector;
    float currentScale = 1.0f;
    void showPresetMenu();
    void applyScale(float newScale);

    // OBJ import
    juce::TextButton objLoadBtn{ "OBJ" };

    // Debug bypass buttons
    juce::TextButton bypassOscBtn{ "MUTE" };
    juce::TextButton bypassFXBtn{ "RAW" };

    // Collapsible MIDI keyboard
    juce::MidiKeyboardComponent midiKeyboard;
    juce::TextButton keyboardToggle{ "KB" };
    bool keyboardVisible = false;

    // 3D Viewport (persistent, left column)
    SDFViewport3D viewport3D;

    // Waveform scope (persistent, below viewport)
    WaveformScope waveformScope;

    // Tab bar + 5 tab panels (right column)
    TabBar tabBar;
    ShapeTab shapeTab;
    OscTab oscTab;
    FXTab fxTab;
    ModTab modTab;
    VisTab visTab;

    // Bottom strip (persistent)
    BottomStrip bottomStrip;

    // Scan mode overlay (bottom-right of viewport)
    ScanModeOverlay scanModeOverlay;

    // Voice activity meter
    int lastVoiceCount = 0;
    juce::Rectangle<int> voiceMeterBounds;

    // Tab switching
    void switchTab(int tabIndex);

    // Presets
    void applyPresetResources();

    // Mod routing
    std::vector<ArcKnob*> modTargetKnobs;
    ArcKnob* currentModHighlight = nullptr;
    ModSource currentDragSource = ModSource::Envelope;
    std::vector<ArcKnob::ModIndicator> modIndicatorScratch;
    void setupModRouting();
    void updateModDepthDisplays();
    void updateScanLabels(int mode);
    ArcKnob* findKnobAt(juce::Point<int> pos);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SDFSynthEditor)
};
