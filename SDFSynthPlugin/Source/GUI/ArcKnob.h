#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/ModulationMatrix.h"
#include <vector>

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
    void setModTargetEnabled(bool enabled) { isModTarget = enabled; }
    void setModDragHighlight(bool highlighted);

    // Multi-source mod indicators
    struct ModIndicator
    {
        ModSource source;
        float depth = 0.f;
        float liveValue = 0.f; // source value * depth
        juce::Colour colour;
    };

    void setModIndicators(const std::vector<ModIndicator>& indicators);
    void updateModLiveValues(const ModulationMatrix& mm);
    bool hasModRoutes() const { return !modIndicators.empty(); }

    // Legacy single-source API (kept for backward compat during transition)
    void setModDepth(float depth);
    float getModDepth() const { return modIndicators.empty() ? 0.f : modIndicators[0].depth; }
    void setModLiveOffset(float offset);

    // Callbacks
    std::function<void(ModSource source, float depth)> onModDepthChanged;
    std::function<void(ModSource source)> onModRouteRemoved;

    static juce::Colour getModSourceColour(ModSource src);

private:
    juce::Slider slider;
    juce::Label nameLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::Colour knobColour;
    juce::String labelText;
    juce::String paramId;
    std::vector<ModIndicator> modIndicators; // up to ~4 visible
    float modLiveOffset = 0.f;
    bool isModTarget = false;
    bool modDragHighlighted = false;
    bool modIconDragging = false;
    int dragIndicatorIndex = -1;
    float modDragStartDepth = 0.f;
    int modDragStartY = 0;
};
