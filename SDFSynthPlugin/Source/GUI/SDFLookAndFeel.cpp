#include "SDFLookAndFeel.h"

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
    setColour(juce::ComboBox::backgroundColourId, bgColour);
    setColour(juce::ComboBox::textColourId, mutedText);
    setColour(juce::ComboBox::outlineColourId, panelBorderCol);
    setColour(juce::PopupMenu::backgroundColourId, panelBg);
    setColour(juce::PopupMenu::textColourId, mutedText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, primaryAccent.withAlpha(0.15f));
    setColour(juce::PopupMenu::highlightedTextColourId, primaryAccent);
}

void SDFLookAndFeel::drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                               const juce::String& title, juce::Colour titleColour)
{
    auto bf = bounds.toFloat();
    bool collapsed = bf.getHeight() < scaled(36.f);
    float cr = collapsed ? juce::jmin(innerCorner, bf.getHeight() * 0.25f) : cornerRadius;

    if (collapsed)
    {
        // Collapsed: simple flat bar with subtle accent tint
        g.setColour(panelBgLight);
        g.fillRoundedRectangle(bf, cr);
        g.setColour(titleColour.withAlpha(0.06f));
        g.fillRoundedRectangle(bf, cr);
        g.setColour(panelBorderCol);
        g.drawRoundedRectangle(bf, cr, 1.f);

        if (title.isNotEmpty())
        {
            auto font = juce::Font(juce::Font::getDefaultSansSerifFontName(), scaled(9.f), juce::Font::bold);
            font.setExtraKerningFactor(0.08f);
            g.setFont(font);
            g.setColour(titleColour);
            g.drawText(title.toUpperCase(), bounds.reduced(8, 0),
                       juce::Justification::centredLeft);
        }
        return;
    }

    // Expanded panel: full layered depth

    // 1. Drop shadow
    g.setColour(shadowColour);
    g.fillRoundedRectangle(bf.translated(0.f, 1.5f), cr);

    // 2. Surface fill
    g.setColour(panelBgLight);
    g.fillRoundedRectangle(bf, cr);

    // 3. Top gradient highlight (5% white fading over top 30%)
    {
        juce::ColourGradient grad(gradientHighlight, bf.getX(), bf.getY(),
                                  gradientHighlight.withAlpha(0.f), bf.getX(), bf.getY() + bf.getHeight() * 0.3f,
                                  false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bf, cr);
    }

    // 4. Border
    g.setColour(panelBorderCol);
    g.drawRoundedRectangle(bf, cr, 1.f);

    // 5. Gradient header (accent at 8% alpha fading to 0%, clipped to top rounded corners)
    if (title.isNotEmpty())
    {
        float headerH = scaled(22.f);
        juce::Path headerClip;
        headerClip.addRoundedRectangle(bf.getX(), bf.getY(), bf.getWidth(), headerH,
                                        cr, cr, true, true, false, false);
        g.saveState();
        g.reduceClipRegion(headerClip);
        juce::ColourGradient hdrGrad(titleColour.withAlpha(0.08f), bf.getX(), bf.getY(),
                                      titleColour.withAlpha(0.f), bf.getX(), bf.getY() + headerH,
                                      false);
        g.setGradientFill(hdrGrad);
        g.fillRect(bf.getX(), bf.getY(), bf.getWidth(), headerH);
        g.restoreState();

        // 6. Title text: uppercase, kerned, bold
        auto font = juce::Font(juce::Font::getDefaultSansSerifFontName(), scaled(9.f), juce::Font::bold);
        font.setExtraKerningFactor(0.08f);
        g.setFont(font);
        g.setColour(titleColour);
        g.drawText(title.toUpperCase(), bounds.reduced(8, 0).removeFromTop(static_cast<int>(headerH)),
                   juce::Justification::centredLeft);
    }
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

    // 1. Center well (depth illusion)
    float wellRadius = radius * 0.45f;
    g.setColour(bgColour);
    g.fillEllipse(centreX - wellRadius, centreY - wellRadius, wellRadius * 2.f, wellRadius * 2.f);

    // 2. Track arc
    juce::Path track;
    track.addCentredArc(centreX, centreY, radius, radius,
                        0.f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(panelBorderCol);
    g.strokePath(track, juce::PathStrokeType(2.f));

    // 3. Value arc colour
    auto colour = slider.findColour(juce::Slider::rotarySliderFillColourId);
    if (colour == juce::Colour()) colour = primaryAccent;

    // 4. Value arc glow (behind main arc)
    {
        juce::Path glowArc;
        glowArc.addCentredArc(centreX, centreY, radius, radius,
                               0.f, rotaryStartAngle, angle, true);
        g.setColour(colour.withAlpha(0.12f));
        g.strokePath(glowArc, juce::PathStrokeType(6.f));
    }

    // 5. Value arc
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, radius, radius,
                           0.f, rotaryStartAngle, angle, true);
    g.setColour(colour);
    g.strokePath(valueArc, juce::PathStrokeType(2.5f));

    // 6. Hover glow
    if (slider.isMouseOverOrDragging())
    {
        juce::Path hoverArc;
        hoverArc.addCentredArc(centreX, centreY, radius, radius,
                                0.f, rotaryStartAngle, angle, true);
        g.setColour(colour.withAlpha(0.18f));
        g.strokePath(hoverArc, juce::PathStrokeType(8.f));
    }

    // Mod depth arc overlay + live indicator (preserved)
    {
        auto modDepthVar = slider.getProperties()["modDepth"];
        float md = modDepthVar.isVoid() ? 0.f : static_cast<float>(modDepthVar);
        if (md != 0.f)
        {
            float modAngle = angle + md * (rotaryEndAngle - rotaryStartAngle);
            modAngle = std::clamp(modAngle, rotaryStartAngle, rotaryEndAngle);
            float modRadius = radius + 3.f;

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

            auto liveVar = slider.getProperties()["modLiveOffset"];
            float liveOff = liveVar.isVoid() ? 0.f : static_cast<float>(liveVar);
            if (std::abs(liveOff) > 0.001f)
            {
                float liveAngle = angle + liveOff * (rotaryEndAngle - rotaryStartAngle);
                liveAngle = std::clamp(liveAngle, rotaryStartAngle, rotaryEndAngle);

                juce::Path liveArc;
                float la0 = std::min(angle, liveAngle);
                float la1 = std::max(angle, liveAngle);
                liveArc.addCentredArc(centreX, centreY, modRadius, modRadius,
                                      0.f, la0, la1, true);
                g.setColour(modColour.withAlpha(0.7f));
                g.strokePath(liveArc, juce::PathStrokeType(3.f));

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

    // 7. Dot indicator (replaces pointer line)
    {
        bool hovering = slider.isMouseOverOrDragging();
        float dotRadius = hovering ? 4.f : 3.f;
        float dotDist = radius * 0.7f;
        float dotX = centreX + dotDist * std::cos(angle - juce::MathConstants<float>::halfPi);
        float dotY = centreY + dotDist * std::sin(angle - juce::MathConstants<float>::halfPi);

        // Glow ring behind dot
        g.setColour(colour.withAlpha(hovering ? 0.25f : 0.12f));
        float glowR = dotRadius + 3.f;
        g.fillEllipse(dotX - glowR, dotY - glowR, glowR * 2.f, glowR * 2.f);

        // Filled dot
        g.setColour(colour);
        g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.f, dotRadius * 2.f);
    }
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

        // Glow shadow ring
        g.setColour(colour.withAlpha(0.10f));
        g.fillRoundedRectangle(bounds.expanded(1.f), innerCorner + 1.f);

        // Fill
        g.setColour(colour.withAlpha(0.20f));
        g.fillRoundedRectangle(bounds, innerCorner);

        // Border
        g.setColour(colour.withAlpha(0.80f));
        g.drawRoundedRectangle(bounds, innerCorner, 1.f);
    }
    else
    {
        // Well effect: darkest layer fill
        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds, innerCorner);

        if (highlighted)
        {
            g.setColour(primaryAccent.withAlpha(0.25f));
            g.drawRoundedRectangle(bounds, innerCorner, 1.5f);
        }
        else
        {
            g.setColour(panelBorderCol);
            g.drawRoundedRectangle(bounds, innerCorner, 1.f);
        }
    }
}

void SDFLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                     bool, bool)
{
    auto colour = button.getToggleState()
        ? button.findColour(juce::TextButton::textColourOnId)
        : button.findColour(juce::TextButton::textColourOffId);
    g.setColour(colour);
    auto font = juce::Font(juce::Font::getDefaultSansSerifFontName(), scaled(9.f), juce::Font::bold);
    font.setExtraKerningFactor(0.04f);
    g.setFont(font);
    g.drawText(button.getButtonText().toUpperCase(), button.getLocalBounds(),
               juce::Justification::centred);
}

void SDFLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                   int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0.f, 0.f, static_cast<float>(width), static_cast<float>(height));

    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, innerCorner);

    auto borderCol = (box.hasKeyboardFocus(true) || isButtonDown) ? primaryAccent.withAlpha(0.5f) : panelBorderCol;
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds.reduced(0.5f), innerCorner, 1.f);

    // Triangle arrow
    float arrowX = static_cast<float>(width) - 14.f;
    float arrowY = static_cast<float>(height) * 0.5f;
    juce::Path arrow;
    arrow.addTriangle(arrowX - 4.f, arrowY - 2.5f,
                      arrowX + 4.f, arrowY - 2.5f,
                      arrowX, arrowY + 2.5f);
    g.setColour(mutedText);
    g.fillPath(arrow);
}

juce::Label* SDFLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* label = LookAndFeel_V4::createSliderTextBox(slider);
    auto font = juce::Font(juce::Font::getDefaultMonospacedFontName(), scaled(10.f), 0);
    label->setFont(font);
    return label;
}

void SDFLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.setColour(panelBg);
    g.fillRoundedRectangle(0.f, 0.f, static_cast<float>(width), static_cast<float>(height), innerCorner);
    g.setColour(panelBorderCol);
    g.drawRoundedRectangle(0.5f, 0.5f, static_cast<float>(width) - 1.f, static_cast<float>(height) - 1.f, innerCorner, 1.f);
}

void SDFLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                        bool isSeparator, bool isActive, bool isHighlighted,
                                        bool isTicked, bool,
                                        const juce::String& text, const juce::String&,
                                        const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        auto sep = area.reduced(8, 0).toFloat();
        g.setColour(panelBorderCol);
        g.drawLine(sep.getX(), sep.getCentreY(), sep.getRight(), sep.getCentreY(), 0.5f);
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour(primaryAccent.withAlpha(0.15f));
        g.fillRoundedRectangle(area.toFloat().reduced(2.f, 0.f), 4.f);
    }

    auto textColour = isHighlighted ? primaryAccent : (isActive ? mutedText : mutedText.withAlpha(0.4f));
    if (isTicked) textColour = primaryAccent;

    g.setColour(textColour);
    auto font = juce::Font(juce::Font::getDefaultSansSerifFontName(), scaled(10.f), 0);
    g.setFont(font);
    g.drawText(text, area.reduced(10, 0), juce::Justification::centredLeft);

    if (isTicked)
    {
        g.setColour(primaryAccent);
        g.fillEllipse(static_cast<float>(area.getRight()) - 14.f,
                      static_cast<float>(area.getCentreY()) - 2.5f, 5.f, 5.f);
    }
}
