#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class ArcKnob : public juce::Component
{
public:
    ArcKnob(juce::AudioProcessorValueTreeState& apvts,
            const juce::String& paramId,
            const juce::String& label,
            juce::Colour colour);
    ~ArcKnob() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void setLabel(const juce::String& newLabel);
    void setTooltipText(const juce::String& tip) { slider.setTooltip(tip); }

private:
    juce::Slider slider;
    juce::Label nameLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::Colour knobColour;
    juce::String labelText;
};
