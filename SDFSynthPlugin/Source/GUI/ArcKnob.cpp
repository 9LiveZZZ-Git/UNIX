#include "ArcKnob.h"
#include "SDFLookAndFeel.h"
#include "Utility/Constants.h"

ArcKnob::ArcKnob(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& pid,
                   const juce::String& label,
                   juce::Colour colour)
    : knobColour(colour), labelText(label), paramId(pid)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 12);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 7.f / 6.f,
                               juce::MathConstants<float>::pi * 17.f / 6.f, true);
    slider.setColour(juce::Slider::rotarySliderFillColourId, colour);
    slider.setColour(juce::Slider::textBoxTextColourId, colour.withAlpha(0.7f));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    auto labelFont = juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(8.f), 0);
    labelFont.setExtraKerningFactor(0.06f);
    nameLabel.setText(label.toUpperCase(), juce::dontSendNotification);
    nameLabel.setFont(labelFont);
    nameLabel.setColour(juce::Label::textColourId, SDFLookAndFeel::mutedText);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(nameLabel);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, pid, slider);
}

ArcKnob::~ArcKnob() = default;

void ArcKnob::setLabel(const juce::String& newLabel)
{
    labelText = newLabel;
    nameLabel.setText(newLabel.toUpperCase(), juce::dontSendNotification);
}

void ArcKnob::setModDepth(float depth)
{
    // Legacy single-source: set as envelope route
    if (std::abs(depth) < 0.001f)
    {
        modIndicators.clear();
    }
    else
    {
        if (modIndicators.empty())
            modIndicators.push_back({ ModSource::Envelope, depth, 0.f, getModSourceColour(ModSource::Envelope) });
        else
            modIndicators[0].depth = depth;
    }
    slider.getProperties().set("modDepth", depth);
    repaint();
}

void ArcKnob::setModIndicators(const std::vector<ModIndicator>& indicators)
{
    modIndicators = indicators;
    // Set legacy property for LookAndFeel compatibility
    float totalDepth = 0.f;
    for (auto& ind : modIndicators)
        totalDepth += ind.depth;
    slider.getProperties().set("modDepth", totalDepth);
    repaint();
}

void ArcKnob::updateModLiveValues(const ModulationMatrix& mm)
{
    float totalOffset = 0.f;
    bool changed = false;
    for (auto& ind : modIndicators)
    {
        float newLive = mm.getSourceValue(ind.source) * ind.depth;
        if (std::abs(newLive - ind.liveValue) > 0.001f)
        {
            ind.liveValue = newLive;
            changed = true;
        }
        totalOffset += newLive;
    }
    if (changed)
    {
        modLiveOffset = totalOffset;
        slider.getProperties().set("modLiveOffset", totalOffset);
        repaint();
    }
}

void ArcKnob::setModDragHighlight(bool highlighted)
{
    modDragHighlighted = highlighted;
    slider.getProperties().set("modDragHighlight", highlighted);
    repaint();
}

void ArcKnob::setModLiveOffset(float offset)
{
    if (std::abs(modLiveOffset - offset) > 0.001f)
    {
        modLiveOffset = offset;
        slider.getProperties().set("modLiveOffset", offset);
        repaint();
    }
}

void ArcKnob::mouseDown(const juce::MouseEvent& e)
{
    if (isModTarget && !modIndicators.empty())
    {
        // Hit test mod indicator dots (stacked vertically in top-right)
        float dotSize = SDFLookAndFeel::scaled(7.f);
        float dotX = static_cast<float>(getWidth()) - dotSize - 2.f;

        for (int i = 0; i < static_cast<int>(modIndicators.size()) && i < 4; ++i)
        {
            float dotY = 2.f + static_cast<float>(i) * (dotSize + 2.f);
            auto hitRect = juce::Rectangle<float>(dotX - 4.f, dotY - 2.f, dotSize + 8.f, dotSize + 4.f);

            if (hitRect.contains(e.position))
            {
                if (e.mods.isPopupMenu())
                {
                    // Right-click: remove this route
                    if (onModRouteRemoved)
                        onModRouteRemoved(modIndicators[static_cast<size_t>(i)].source);
                    return;
                }

                // Left-click: start depth drag for this indicator
                modIconDragging = true;
                dragIndicatorIndex = i;
                modDragStartDepth = modIndicators[static_cast<size_t>(i)].depth;
                modDragStartY = e.getPosition().y;
                return;
            }
        }
    }
    Component::mouseDown(e);
}

void ArcKnob::mouseDrag(const juce::MouseEvent& e)
{
    if (modIconDragging && dragIndicatorIndex >= 0 && dragIndicatorIndex < static_cast<int>(modIndicators.size()))
    {
        int deltaY = modDragStartY - e.getPosition().y;
        float deltaDepth = deltaY / 60.f;
        float newDepth = std::clamp(modDragStartDepth + deltaDepth, -1.f, 1.f);
        if (onModDepthChanged)
            onModDepthChanged(modIndicators[static_cast<size_t>(dragIndicatorIndex)].source, newDepth);
        return;
    }
    Component::mouseDrag(e);
}

void ArcKnob::mouseUp(const juce::MouseEvent& e)
{
    if (modIconDragging)
    {
        modIconDragging = false;
        dragIndicatorIndex = -1;
        return;
    }
    Component::mouseUp(e);
}

void ArcKnob::paint(juce::Graphics& g)
{
    if (!isModTarget) return;

    float dotSize = SDFLookAndFeel::scaled(7.f);
    float dotX = static_cast<float>(getWidth()) - dotSize - 2.f;

    if (!modIndicators.empty())
    {
        // Draw stacked colored dots for each active mod source
        for (int i = 0; i < static_cast<int>(modIndicators.size()) && i < 4; ++i)
        {
            float dotY = 2.f + static_cast<float>(i) * (dotSize + 2.f);
            float cx = dotX + dotSize * 0.5f;
            float cy = dotY + dotSize * 0.5f;

            auto& ind = modIndicators[static_cast<size_t>(i)];

            // Background glow
            g.setColour(ind.colour.withAlpha(0.25f));
            g.fillEllipse(cx - 5.f, cy - 5.f, 10.f, 10.f);

            // Filled dot
            g.setColour(ind.colour);
            g.fillEllipse(cx - 3.f, cy - 3.f, 6.f, 6.f);
        }
    }
    else if (modDragHighlighted)
    {
        float cx = dotX + dotSize * 0.5f;
        float cy = 2.f + dotSize * 0.5f;
        g.setColour(juce::Colour(0xFFFF6432).withAlpha(0.9f));
        g.fillEllipse(cx - 4.f, cy - 4.f, 8.f, 8.f);
        g.setColour(SDFLookAndFeel::panelBg);
        g.fillEllipse(cx - 2.f, cy - 2.f, 4.f, 4.f);
    }
    else
    {
        float cx = dotX + dotSize * 0.5f;
        float cy = 2.f + dotSize * 0.5f;
        g.setColour(SDFLookAndFeel::mutedText.withAlpha(0.4f));
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.f, 5.f);
    }
}

void ArcKnob::resized()
{
    auto bounds = getLocalBounds();
    int labelH = SDFLookAndFeel::scaledInt(14);
    nameLabel.setBounds(bounds.removeFromTop(labelH));
    auto labelFont = juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(8.f), 0);
    labelFont.setExtraKerningFactor(0.06f);
    nameLabel.setFont(labelFont);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                           juce::jmax(SDFLookAndFeel::scaledInt(48), getWidth() - SDFLookAndFeel::scaledInt(8)),
                           SDFLookAndFeel::scaledInt(12));
    slider.setBounds(bounds);
}

juce::Colour ArcKnob::getModSourceColour(ModSource src)
{
    switch (src)
    {
        case ModSource::Envelope:   return juce::Colour(sdfColour::modEnvelope);
        case ModSource::LFO1:      return juce::Colour(sdfColour::modLFO1);
        case ModSource::LFO2:      return juce::Colour(sdfColour::modLFO2);
        case ModSource::ModWheel:  return juce::Colour(sdfColour::modModWheel);
        case ModSource::Velocity:  return juce::Colour(sdfColour::modVelocity);
        case ModSource::Aftertouch:return juce::Colour(sdfColour::modAftertouch);
        case ModSource::KeyTrack:  return juce::Colour(sdfColour::modKeyTrack);
        case ModSource::Random:    return juce::Colour(sdfColour::modRandom);
        case ModSource::Macro1:
        case ModSource::Macro2:
        case ModSource::Macro3:
        case ModSource::Macro4:    return juce::Colour(sdfColour::modMacro);
        default:                   return juce::Colour(sdfColour::modEnvelope);
    }
}
