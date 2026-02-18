#include "ArcKnob.h"
#include "SDFLookAndFeel.h"

ArcKnob::ArcKnob(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramId,
                   const juce::String& label,
                   juce::Colour colour)
    : knobColour(colour), labelText(label)
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
    addAndMakeVisible(nameLabel);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramId, slider);
}

ArcKnob::~ArcKnob() = default;

void ArcKnob::setLabel(const juce::String& newLabel)
{
    labelText = newLabel;
    nameLabel.setText(newLabel.toUpperCase(), juce::dontSendNotification);
}

void ArcKnob::paint(juce::Graphics&) {}

void ArcKnob::resized()
{
    auto bounds = getLocalBounds();
    nameLabel.setBounds(bounds.removeFromTop(12));
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                           juce::jmax(40, getWidth() - 10), 12);
    slider.setBounds(bounds);
}
