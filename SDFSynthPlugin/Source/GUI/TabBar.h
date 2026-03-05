#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "SDFLookAndFeel.h"

class TabBar : public juce::Component
{
public:
    TabBar()
    {
        juce::StringArray names = { "SHAPE", "OSC", "FX", "MOD", "VIS" };
        for (int i = 0; i < names.size(); ++i)
        {
            auto* btn = tabs.add(new juce::TextButton(names[i]));
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(7777);
            btn->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            btn->setColour(juce::TextButton::buttonOnColourId, SDFLookAndFeel::panelBg);
            btn->setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
            btn->setColour(juce::TextButton::textColourOnId, SDFLookAndFeel::primaryAccent);
            btn->onClick = [this, i]()
            {
                activeTab = i;
                if (onTabChanged)
                    onTabChanged(i);
            };
            addAndMakeVisible(btn);
        }

        tabs[0]->setToggleState(true, juce::dontSendNotification);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        int btnW = bounds.getWidth() / tabs.size();
        for (auto* btn : tabs)
            btn->setBounds(bounds.removeFromLeft(btnW));
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(SDFLookAndFeel::panelBg);
        g.fillRoundedRectangle(bounds, 4.f);

        // Active tab underline
        if (activeTab >= 0 && activeTab < tabs.size())
        {
            auto tabBounds = tabs[activeTab]->getBounds().toFloat();
            g.setColour(SDFLookAndFeel::primaryAccent);
            g.fillRect(tabBounds.getX() + 4.f, tabBounds.getBottom() - 2.f,
                       tabBounds.getWidth() - 8.f, 2.f);
        }
    }

    void setActiveTab(int idx)
    {
        if (idx >= 0 && idx < tabs.size())
        {
            activeTab = idx;
            tabs[idx]->setToggleState(true, juce::dontSendNotification);
        }
    }

    int getActiveTab() const { return activeTab; }

    std::function<void(int)> onTabChanged;

private:
    juce::OwnedArray<juce::TextButton> tabs;
    int activeTab = 0;
};
