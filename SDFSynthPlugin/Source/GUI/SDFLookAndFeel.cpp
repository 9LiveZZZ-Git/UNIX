#include "SDFLookAndFeel.h"

void SDFLookAndFeel::drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                               const juce::String& title, juce::Colour titleColour)
{
    auto bf = bounds.toFloat();
    g.setColour(panelBgLight);
    g.fillRoundedRectangle(bf, 6.f);
    g.setColour(panelBorderCol);
    g.drawRoundedRectangle(bf, 6.f, 1.f);

    if (title.isNotEmpty())
    {
        g.setColour(titleColour);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.f, juce::Font::bold));
        g.drawText(title, bounds.reduced(8, 0).removeFromTop(18),
                   juce::Justification::centredLeft);
    }
}

SDFLookAndFeel::SDFLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, bgColour);
    setColour(juce::Slider::textBoxTextColourId, primaryAccent);
    setColour(juce::Slider::textBoxBackgroundColourId, panelBg);
    setColour(juce::Slider::textBoxOutlineColourId, borderColour);
    setColour(juce::Label::textColourId, mutedText);
    setColour(juce::TextButton::buttonColourId, panelBg);
    setColour(juce::TextButton::textColourOffId, mutedText);
    setColour(juce::TextButton::textColourOnId, primaryAccent);
}

void SDFLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle,
                                       float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                          static_cast<float>(width), static_cast<float>(height));
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.f - 2.f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Track arc
    juce::Path track;
    track.addCentredArc(centreX, centreY, radius, radius,
                        0.f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(borderColour);
    g.strokePath(track, juce::PathStrokeType(1.5f));

    // Value arc
    auto colour = slider.findColour(juce::Slider::rotarySliderFillColourId);
    if (colour == juce::Colour()) colour = primaryAccent;
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius, radius,
                           0.f, rotaryStartAngle, angle, true);
    g.setColour(colour);
    g.strokePath(valueArc, juce::PathStrokeType(1.5f));

    // Hover glow: brighten arc when mouse is over the slider
    if (slider.isMouseOverOrDragging())
    {
        juce::Path glowArc;
        glowArc.addCentredArc(centreX, centreY, radius, radius,
                              0.f, rotaryStartAngle, angle, true);
        g.setColour(colour.withAlpha(0.15f));
        g.strokePath(glowArc, juce::PathStrokeType(4.f));
    }

    // Pointer line
    juce::Path pointer;
    auto pointerLength = radius * 0.6f;
    auto pointerWidth = slider.isMouseOverOrDragging() ? 2.f : 1.5f;
    pointer.startNewSubPath(centreX, centreY);
    pointer.lineTo(centreX + pointerLength * std::cos(angle - juce::MathConstants<float>::halfPi),
                   centreY + pointerLength * std::sin(angle - juce::MathConstants<float>::halfPi));
    g.setColour(colour);
    g.strokePath(pointer, juce::PathStrokeType(pointerWidth));
}

void SDFLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour&,
                                            bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    bool toggled = button.getToggleState();

    if (toggled || down)
    {
        auto colour = button.findColour(juce::TextButton::buttonOnColourId);
        if (colour == juce::Colour()) colour = primaryAccent;
        g.setColour(colour.withAlpha(0.25f));
        g.fillRoundedRectangle(bounds, 3.f);
        g.setColour(colour);
        g.drawRoundedRectangle(bounds, 3.f, 1.f);
    }
    else
    {
        g.setColour(panelBg);
        g.fillRoundedRectangle(bounds, 3.f);
        g.setColour(highlighted ? primaryAccent.withAlpha(0.3f) : borderColour);
        g.drawRoundedRectangle(bounds, 3.f, 1.f);
    }
}

void SDFLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                     bool, bool)
{
    auto colour = button.getToggleState()
        ? button.findColour(juce::TextButton::textColourOnId)
        : button.findColour(juce::TextButton::textColourOffId);
    g.setColour(colour);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds(),
               juce::Justification::centred);
}
