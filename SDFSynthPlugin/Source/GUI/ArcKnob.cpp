#include "ArcKnob.h"
#include "SDFLookAndFeel.h"

ArcKnob::ArcKnob(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& pid,
                   const juce::String& label,
                   juce::Colour colour)
    : knobColour(colour), labelText(label), paramId(pid)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 12);
    slider.setColour(juce::Slider::rotarySliderFillColourId, colour);
    slider.setColour(juce::Slider::textBoxTextColourId, colour.withAlpha(0.7f));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    nameLabel.setText(label.toUpperCase(), juce::dontSendNotification);
    nameLabel.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.f, 0));
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
    modDepth = depth;
    slider.getProperties().set("modDepth", depth);
    repaint();
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
    if (isModTarget && modDepth != 0.f)
    {
        // Hit test the mod indicator area (top-right corner, generous 16px target)
        float hitX = static_cast<float>(getWidth()) - 16.f;
        auto hitRect = juce::Rectangle<float>(hitX, 0.f, 16.f, 16.f);

        if (hitRect.contains(e.position))
        {
            if (e.mods.isPopupMenu())
            {
                // Right-click: remove mod route
                if (onModRouteRemoved) onModRouteRemoved();
                return;
            }

            // Left-click: start depth drag
            modIconDragging = true;
            modDragStartDepth = modDepth;
            modDragStartY = e.getPosition().y;
            return;
        }
    }
    Component::mouseDown(e);
}

void ArcKnob::mouseDrag(const juce::MouseEvent& e)
{
    if (modIconDragging)
    {
        int deltaY = modDragStartY - e.getPosition().y; // up = positive = increase
        float deltaDepth = deltaY / 60.f; // 60px drag = full range
        float newDepth = std::clamp(modDragStartDepth + deltaDepth, -1.f, 1.f);
        if (onModDepthChanged) onModDepthChanged(newDepth);
        return;
    }
    Component::mouseDrag(e);
}

void ArcKnob::mouseUp(const juce::MouseEvent& e)
{
    if (modIconDragging)
    {
        modIconDragging = false;
        return;
    }
    Component::mouseUp(e);
}

void ArcKnob::paint(juce::Graphics& g)
{
    if (!isModTarget) return;

    // Mod target indicator — dot in top-right corner of label area
    float iconSize = 7.f;
    float ix = static_cast<float>(getWidth()) - iconSize - 2.f;
    float iy = 2.f;
    float cx = ix + iconSize * 0.5f;
    float cy = iy + iconSize * 0.5f;

    if (modDepth != 0.f)
    {
        // Active route: filled circle with depth indicator
        auto col = modDepth > 0.f ? juce::Colour(0xFFFF6432) : juce::Colour(0xFFFF3264);
        g.setColour(col.withAlpha(0.25f));
        g.fillEllipse(cx - 5.f, cy - 5.f, 10.f, 10.f);
        g.setColour(col);
        g.fillEllipse(cx - 3.f, cy - 3.f, 6.f, 6.f);
    }
    else if (modDragHighlighted)
    {
        // Highlighted during drag: bright ring
        g.setColour(juce::Colour(0xFFFF6432).withAlpha(0.9f));
        g.fillEllipse(cx - 4.f, cy - 4.f, 8.f, 8.f);
        g.setColour(SDFLookAndFeel::panelBg);
        g.fillEllipse(cx - 2.f, cy - 2.f, 4.f, 4.f);
    }
    else
    {
        // Available target: subtle dot
        g.setColour(SDFLookAndFeel::mutedText.withAlpha(0.4f));
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.f, 5.f);
    }
}

void ArcKnob::resized()
{
    auto bounds = getLocalBounds();
    nameLabel.setBounds(bounds.removeFromTop(12));
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                           juce::jmax(40, getWidth() - 10), 12);
    slider.setBounds(bounds);
}
