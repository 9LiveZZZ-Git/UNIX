#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SDFLookAndFeel.h"

class SegmentedControl : public juce::Component,
                          public juce::AudioProcessorValueTreeState::Listener
{
public:
    SegmentedControl(juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& paramId,
                     const juce::StringArray& labels,
                     juce::Colour activeColour = SDFLookAndFeel::primaryAccent)
        : apvtsRef(apvts), paramID(paramId), accentColour(activeColour)
    {
        for (int i = 0; i < labels.size(); ++i)
        {
            auto* btn = buttons.add(new juce::TextButton(labels[i]));
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(6000 + paramId.hashCode());
            btn->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            btn->setColour(juce::TextButton::buttonOnColourId, accentColour.withAlpha(0.2f));
            btn->setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
            btn->setColour(juce::TextButton::textColourOnId, accentColour);
            btn->onClick = [this, i]()
            {
                if (auto* param = apvtsRef.getParameter(paramID))
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
            };
            addAndMakeVisible(btn);
        }

        if (auto* paramVal = apvts.getRawParameterValue(paramId))
        {
            int initVal = static_cast<int>(paramVal->load());
            if (initVal >= 0 && initVal < buttons.size())
                buttons[initVal]->setToggleState(true, juce::dontSendNotification);
        }

        apvts.addParameterListener(paramId, this);
    }

    ~SegmentedControl() override
    {
        apvtsRef.removeParameterListener(paramID, this);
    }

    void parameterChanged(const juce::String&, float newValue) override
    {
        int idx = static_cast<int>(newValue);
        juce::MessageManager::callAsync([this, idx]()
        {
            if (idx >= 0 && idx < buttons.size())
                buttons[idx]->setToggleState(true, juce::dontSendNotification);
        });
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        if (buttons.size() == 0) return;
        int btnW = bounds.getWidth() / buttons.size();
        for (auto* btn : buttons)
            btn->setBounds(bounds.removeFromLeft(btnW));
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, 3.f, 1.f);
    }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramID;
    juce::Colour accentColour;
    juce::OwnedArray<juce::TextButton> buttons;
};
