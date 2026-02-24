#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SDFLookAndFeel.h"

class ADSRDisplay : public juce::Component,
                    public juce::AudioProcessorValueTreeState::Listener
{
public:
    ADSRDisplay(juce::AudioProcessorValueTreeState& apvts);
    ~ADSRDisplay() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Mod drag handle callbacks
    std::function<void()> onModDragStarted;
    std::function<void(juce::Point<int>)> onModDragging;
    std::function<void(juce::Point<int>)> onModDragEnded;
    bool isModDragActive() const { return modDragging; }
    juce::Point<int> getModDragPos() const { return modDragPos; }
    juce::Point<int> getModHandleCentre() const;

private:
    juce::AudioProcessorValueTreeState& apvts;

    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.8f;
    float release = 0.3f;

    enum DragTarget { None, Attack, Decay, Release };
    DragTarget currentDrag = None;
    bool modDragging = false;
    juce::Point<int> modDragPos;

    // Layout helpers
    struct EnvPoints
    {
        juce::Point<float> start, attackPeak, decayEnd, sustainEnd, releaseEnd;
    };
    EnvPoints computePoints(juce::Rectangle<float> area) const;
    DragTarget hitTest(juce::Point<float> pos, const EnvPoints& pts) const;
    juce::Rectangle<float> getModHandleRect() const;

    void writeParam(const juce::String& id, float value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSRDisplay)
};
