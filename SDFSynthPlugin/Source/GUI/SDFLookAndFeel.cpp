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

    // Mod depth arc overlay + live indicator
    {
        auto modDepthVar = slider.getProperties()["modDepth"];
        float md = modDepthVar.isVoid() ? 0.f : static_cast<float>(modDepthVar);
        if (md != 0.f)
        {
            float modAngle = angle + md * (rotaryEndAngle - rotaryStartAngle);
            modAngle = std::clamp(modAngle, rotaryStartAngle, rotaryEndAngle);
            float modRadius = radius + 3.f;

            // Range arc (shows full mod range, dimmer)
            juce::Path modArc;
            float arcStart = std::min(angle, modAngle);
            float arcEnd = std::max(angle, modAngle);
            modArc.addCentredArc(centreX, centreY, modRadius, modRadius,
                                 0.f, arcStart, arcEnd, true);

            auto modColour = md > 0.f ? juce::Colour(0xFFFF6432) : juce::Colour(0xFFFF3264);
            g.setColour(modColour.withAlpha(0.12f));
            g.strokePath(modArc, juce::PathStrokeType(5.f));
            g.setColour(modColour.withAlpha(0.5f));
            g.strokePath(modArc, juce::PathStrokeType(2.f));

            // Live modulation indicator (animated dot at current modulated position)
            auto liveVar = slider.getProperties()["modLiveOffset"];
            float liveOff = liveVar.isVoid() ? 0.f : static_cast<float>(liveVar);
            if (std::abs(liveOff) > 0.001f)
            {
                float liveAngle = angle + liveOff * (rotaryEndAngle - rotaryStartAngle);
                liveAngle = std::clamp(liveAngle, rotaryStartAngle, rotaryEndAngle);

                // Filled arc from base to live position
                juce::Path liveArc;
                float la0 = std::min(angle, liveAngle);
                float la1 = std::max(angle, liveAngle);
                liveArc.addCentredArc(centreX, centreY, modRadius, modRadius,
                                      0.f, la0, la1, true);
                g.setColour(modColour.withAlpha(0.7f));
                g.strokePath(liveArc, juce::PathStrokeType(3.f));

                // Bright dot at live position
                float dotX = centreX + modRadius * std::cos(liveAngle - juce::MathConstants<float>::halfPi);
                float dotY = centreY + modRadius * std::sin(liveAngle - juce::MathConstants<float>::halfPi);
                g.setColour(modColour.withAlpha(0.3f));
                g.fillEllipse(dotX - 4.5f, dotY - 4.5f, 9.f, 9.f);
                g.setColour(modColour);
                g.fillEllipse(dotX - 2.5f, dotY - 2.5f, 5.f, 5.f);
            }
        }
    }

    // Mod drag highlight background
    {
        auto highlightVar = slider.getProperties()["modDragHighlight"];
        bool highlighted = highlightVar.isVoid() ? false : static_cast<bool>(highlightVar);
        if (highlighted)
        {
            g.setColour(juce::Colour(0xFFFF6432).withAlpha(0.12f));
            g.fillEllipse(centreX - radius - 4.f, centreY - radius - 4.f,
                          (radius + 4.f) * 2.f, (radius + 4.f) * 2.f);
        }
    }

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
