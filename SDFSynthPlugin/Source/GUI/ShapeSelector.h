#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class ShapeSelector : public juce::Component,
                       public juce::AudioProcessorValueTreeState::Listener
{
public:
    ShapeSelector(juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramId,
                  const juce::String& label);
    ~ShapeSelector() override;

    void resized() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    juce::Label titleLabel;
    juce::OwnedArray<juce::TextButton> buttons;
    juce::String paramId;
    juce::AudioProcessorValueTreeState& apvtsRef;

    void updateSelection(int index);
};
