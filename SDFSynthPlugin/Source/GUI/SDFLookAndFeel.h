#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class SDFLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SDFLookAndFeel();

    // Colors
    static inline const juce::Colour bgColour         { 0xff050c0c };
    static inline const juce::Colour panelBg           { 0xff0e2020 };
    static inline const juce::Colour borderColour      { 0xff2a5050 };
    static inline const juce::Colour primaryAccent     { 0xff00ffff };
    static inline const juce::Colour secondaryAccent   { 0xffff6432 };
    static inline const juce::Colour tertiaryAccent    { 0xff00ff88 };
    static inline const juce::Colour mutedText         { 0xff708090 };
    static inline const juce::Colour inactiveBorder    { 0xff2a5050 };
    static inline const juce::Colour panelBgLight      { 0xff0a1818 };
    static inline const juce::Colour panelBorderCol    { 0xff1a3838 };
    static inline const juce::Colour panelTitleColour  { 0xff4a7070 };

    // Depth & glow
    static inline const juce::Colour shadowColour      { 0x1a000000 };
    static inline const juce::Colour gradientHighlight  { 0x0dffffff };
    static inline const juce::Colour meterYellow       { 0xffffcc44 };
    static inline const juce::Colour meterRed          { 0xffff4444 };
    static constexpr float cornerRadius = 8.f;
    static constexpr float innerCorner  = 6.f;

    // UI scaling
    static inline float scaleFactor = 1.0f;
    static float scaled(float basePx)  { return basePx * scaleFactor; }
    static int scaledInt(int basePx)   { return juce::roundToInt(basePx * scaleFactor); }

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

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;

    juce::Label* createSliderTextBox(juce::Slider&) override;

    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;
};
