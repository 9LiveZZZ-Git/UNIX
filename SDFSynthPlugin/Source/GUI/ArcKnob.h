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
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void setLabel(const juce::String& newLabel);
    void setTooltipText(const juce::String& tip) { slider.setTooltip(tip); }

    // Modulation overlay
    const juce::String& getParameterID() const { return paramId; }
    void setModDepth(float depth);
    float getModDepth() const { return modDepth; }
    void setModTargetEnabled(bool enabled) { isModTarget = enabled; }
    void setModDragHighlight(bool highlighted);
    void setModLiveOffset(float offset);

    // Callbacks
    std::function<void(float)> onModDepthChanged;
    std::function<void()> onModRouteRemoved;

private:
    juce::Slider slider;
    juce::Label nameLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::Colour knobColour;
    juce::String labelText;
    juce::String paramId;
    float modDepth = 0.f;
    float modLiveOffset = 0.f;
    bool isModTarget = false;
    bool modDragHighlighted = false;
    bool modIconDragging = false;
    float modDragStartDepth = 0.f;
    int modDragStartY = 0;
};
