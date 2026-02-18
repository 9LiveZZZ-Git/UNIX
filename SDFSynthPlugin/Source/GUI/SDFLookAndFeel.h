#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class SDFLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SDFLookAndFeel();

    // Colors
    static inline const juce::Colour bgColour         { 0xff060e0e };
    static inline const juce::Colour panelBg           { 0xff080f0f };
    static inline const juce::Colour borderColour      { 0xff2a4a4a };
    static inline const juce::Colour primaryAccent     { 0xff00ffff };
    static inline const juce::Colour secondaryAccent   { 0xffff6432 };
    static inline const juce::Colour tertiaryAccent    { 0xff00ff88 };
    static inline const juce::Colour mutedText         { 0xff778899 };
    static inline const juce::Colour inactiveBorder    { 0xff2a4a4a };
    static inline const juce::Colour panelBgLight      { 0xff0c1616 };
    static inline const juce::Colour panelBorderCol    { 0xff1e3636 };
    static inline const juce::Colour panelTitleColour  { 0xff4a6a6a };

    static void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                          const juce::String& title, juce::Colour titleColour = panelTitleColour);

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColourToUse,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;
};
