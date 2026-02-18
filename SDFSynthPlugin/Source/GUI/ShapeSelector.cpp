#include "ShapeSelector.h"
#include "SDFLookAndFeel.h"

ShapeSelector::ShapeSelector(juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& pid,
                               const juce::String& label)
    : paramId(pid), apvtsRef(apvts)
{
    titleLabel.setText(label, juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 8.f, 0));
    titleLabel.setColour(juce::Label::textColourId, SDFLookAndFeel::mutedText);
    addAndMakeVisible(titleLabel);

    juce::StringArray names = { "SPH", "BOX", "TOR", "CYL", "OCT", "OBJ" };
    for (int i = 0; i < names.size(); ++i)
    {
        auto* btn = buttons.add(new juce::TextButton(names[i]));
        btn->setClickingTogglesState(true);
        btn->setRadioGroupId(1000 + pid.hashCode());
        btn->setColour(juce::TextButton::buttonOnColourId, SDFLookAndFeel::primaryAccent);
        btn->setColour(juce::TextButton::textColourOnId, SDFLookAndFeel::primaryAccent);
        btn->setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
        btn->onClick = [this, i]() { updateSelection(i); };
        addAndMakeVisible(btn);
    }

    apvts.addParameterListener(paramId, this);

    int initVal = static_cast<int>(apvts.getRawParameterValue(paramId)->load());
    if (initVal >= 0 && initVal < buttons.size())
        buttons[initVal]->setToggleState(true, juce::dontSendNotification);
}

ShapeSelector::~ShapeSelector()
{
    apvtsRef.removeParameterListener(paramId, this);
}

void ShapeSelector::parameterChanged(const juce::String&, float newValue)
{
    int idx = static_cast<int>(newValue);
    juce::MessageManager::callAsync([this, idx]()
    {
        if (idx >= 0 && idx < buttons.size())
            buttons[idx]->setToggleState(true, juce::dontSendNotification);
    });
}

void ShapeSelector::updateSelection(int index)
{
    if (auto* param = apvtsRef.getParameter(paramId))
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(index)));
}

void ShapeSelector::resized()
{
    auto bounds = getLocalBounds();
    titleLabel.setBounds(bounds.removeFromLeft(60));

    if (buttons.size() == 0) return;
    auto btnWidth = bounds.getWidth() / buttons.size();
    for (auto* btn : buttons)
        btn->setBounds(bounds.removeFromLeft(btnWidth));
}
