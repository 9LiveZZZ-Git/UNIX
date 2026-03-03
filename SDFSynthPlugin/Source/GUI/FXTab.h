#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SDFLookAndFeel.h"
#include "ArcKnob.h"

class FXTab : public juce::Component
{
public:
    FXTab(juce::AudioProcessorValueTreeState& apvts)
        : fxDistDriveKnob(apvts, "fxDistDrive", "Drive", SDFLookAndFeel::tertiaryAccent),
          fxDistMixKnob(apvts, "fxDistMix", "Mix", SDFLookAndFeel::tertiaryAccent),
          fxChorusRateKnob(apvts, "fxChorusRate", "Rate", SDFLookAndFeel::tertiaryAccent),
          fxChorusDepthKnob(apvts, "fxChorusDepth", "Depth", SDFLookAndFeel::tertiaryAccent),
          fxChorusMixKnob(apvts, "fxChorusMix", "Mix", SDFLookAndFeel::tertiaryAccent),
          fxDelayTimeKnob(apvts, "fxDelayTime", "Time", SDFLookAndFeel::tertiaryAccent),
          fxDelayFbKnob(apvts, "fxDelayFeedback", "FB", SDFLookAndFeel::tertiaryAccent),
          fxDelayMixKnob(apvts, "fxDelayMix", "Mix", SDFLookAndFeel::tertiaryAccent),
          fxReverbSizeKnob(apvts, "fxReverbSize", "Size", SDFLookAndFeel::tertiaryAccent),
          fxReverbDampKnob(apvts, "fxReverbDamping", "Damp", SDFLookAndFeel::tertiaryAccent),
          fxReverbMixKnob(apvts, "fxReverbMix", "Mix", SDFLookAndFeel::tertiaryAccent)
    {
        // Enable toggles
        auto setupToggle = [&](juce::ToggleButton& btn, const juce::String& paramId,
                                std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attach)
        {
            btn.setColour(juce::ToggleButton::tickColourId, SDFLookAndFeel::tertiaryAccent);
            btn.setColour(juce::ToggleButton::tickDisabledColourId, SDFLookAndFeel::mutedText);
            addAndMakeVisible(btn);
            attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramId, btn);
        };

        setupToggle(fxDistEnableBtn, "fxDistEnable", fxDistEnableAttach);
        setupToggle(fxChorusEnableBtn, "fxChorusEnable", fxChorusEnableAttach);
        setupToggle(fxDelayEnableBtn, "fxDelayEnable", fxDelayEnableAttach);
        setupToggle(fxReverbEnableBtn, "fxReverbEnable", fxReverbEnableAttach);

        addAndMakeVisible(fxDistDriveKnob);
        addAndMakeVisible(fxDistMixKnob);
        addAndMakeVisible(fxChorusRateKnob);
        addAndMakeVisible(fxChorusDepthKnob);
        addAndMakeVisible(fxChorusMixKnob);
        addAndMakeVisible(fxDelayTimeKnob);
        addAndMakeVisible(fxDelayFbKnob);
        addAndMakeVisible(fxDelayMixKnob);
        addAndMakeVisible(fxReverbSizeKnob);
        addAndMakeVisible(fxReverbDampKnob);
        addAndMakeVisible(fxReverbMixKnob);

        // Distortion type selector
        fxDistTypeSelector.addItem("Soft", 1);
        fxDistTypeSelector.addItem("Hard", 2);
        fxDistTypeSelector.addItem("Fold", 3);
        fxDistTypeSelector.addItem("Crush", 4);
        int initDist = static_cast<int>(apvts.getRawParameterValue("fxDistType")->load());
        fxDistTypeSelector.setSelectedId(initDist + 1, juce::dontSendNotification);
        fxDistTypeSelector.onChange = [this, &apvts]()
        {
            int id = fxDistTypeSelector.getSelectedId();
            if (id > 0)
                if (auto* p = apvts.getParameter("fxDistType"))
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(id - 1)));
        };
        fxDistTypeSelector.setColour(juce::ComboBox::backgroundColourId, SDFLookAndFeel::panelBg);
        fxDistTypeSelector.setColour(juce::ComboBox::textColourId, SDFLookAndFeel::tertiaryAccent);
        fxDistTypeSelector.setColour(juce::ComboBox::outlineColourId, SDFLookAndFeel::borderColour);
        addAndMakeVisible(fxDistTypeSelector);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 2);
        int rowH = juce::jmax(1, bounds.getHeight() / 4);

        // Row 1: Distortion
        {
            auto row = bounds.removeFromTop(rowH);
            fxDistEnableBtn.setBounds(row.removeFromLeft(24).reduced(0, 2));
            fxDistTypeSelector.setBounds(row.removeFromLeft(50).reduced(0, juce::jmax(0, (row.getHeight() - 20) / 2)));
            int kw = row.getWidth() / 2;
            fxDistDriveKnob.setBounds(row.removeFromLeft(kw));
            fxDistMixKnob.setBounds(row);
        }

        // Row 2: Chorus
        {
            auto row = bounds.removeFromTop(rowH);
            fxChorusEnableBtn.setBounds(row.removeFromLeft(24).reduced(0, 2));
            int kw = row.getWidth() / 3;
            fxChorusRateKnob.setBounds(row.removeFromLeft(kw));
            fxChorusDepthKnob.setBounds(row.removeFromLeft(kw));
            fxChorusMixKnob.setBounds(row);
        }

        // Row 3: Delay
        {
            auto row = bounds.removeFromTop(rowH);
            fxDelayEnableBtn.setBounds(row.removeFromLeft(24).reduced(0, 2));
            int kw = row.getWidth() / 3;
            fxDelayTimeKnob.setBounds(row.removeFromLeft(kw));
            fxDelayFbKnob.setBounds(row.removeFromLeft(kw));
            fxDelayMixKnob.setBounds(row);
        }

        // Row 4: Reverb
        {
            auto row = bounds;
            fxReverbEnableBtn.setBounds(row.removeFromLeft(24).reduced(0, 2));
            int kw = row.getWidth() / 3;
            fxReverbSizeKnob.setBounds(row.removeFromLeft(kw));
            fxReverbDampKnob.setBounds(row.removeFromLeft(kw));
            fxReverbMixKnob.setBounds(row);
        }
    }

    void paint(juce::Graphics& g) override
    {
        SDFLookAndFeel::drawPanel(g, getLocalBounds(), "EFFECTS", SDFLookAndFeel::tertiaryAccent);
    }

    // Accessors for mod routing
    ArcKnob& getDistDriveKnob() { return fxDistDriveKnob; }
    ArcKnob& getChorusRateKnob() { return fxChorusRateKnob; }
    ArcKnob& getDelayFbKnob() { return fxDelayFbKnob; }
    ArcKnob& getReverbSizeKnob() { return fxReverbSizeKnob; }

private:
    juce::ToggleButton fxDistEnableBtn{ "D" }, fxChorusEnableBtn{ "C" },
                       fxDelayEnableBtn{ "D" }, fxReverbEnableBtn{ "R" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        fxDistEnableAttach, fxChorusEnableAttach, fxDelayEnableAttach, fxReverbEnableAttach;
    ArcKnob fxDistDriveKnob, fxDistMixKnob;
    ArcKnob fxChorusRateKnob, fxChorusDepthKnob, fxChorusMixKnob;
    ArcKnob fxDelayTimeKnob, fxDelayFbKnob, fxDelayMixKnob;
    ArcKnob fxReverbSizeKnob, fxReverbDampKnob, fxReverbMixKnob;
    juce::ComboBox fxDistTypeSelector;
};
