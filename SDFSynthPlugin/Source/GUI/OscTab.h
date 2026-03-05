#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SDFLookAndFeel.h"
#include "ArcKnob.h"

class OscTab : public juce::Component
{
public:
    OscTab(juce::AudioProcessorValueTreeState& apvts)
        : apvtsRef(apvts),
          oscFoldKnob(apvts, "oscFold", "Fold", SDFLookAndFeel::secondaryAccent),
          oscPDKnob(apvts, "oscPhaseDist", "PD", SDFLookAndFeel::secondaryAccent),
          oscPWKnob(apvts, "oscPW", "PW", SDFLookAndFeel::secondaryAccent),
          oscSyncKnob(apvts, "oscSync", "Sync", SDFLookAndFeel::secondaryAccent),
          uniVoicesKnob(apvts, "unisonVoices", "Voices", SDFLookAndFeel::primaryAccent),
          uniDetuneKnob(apvts, "unisonDetune", "Detune", SDFLookAndFeel::primaryAccent),
          uniSpreadKnob(apvts, "unisonSpread", "Spread", SDFLookAndFeel::primaryAccent),
          uniBlendKnob(apvts, "unisonBlend", "Blend", SDFLookAndFeel::primaryAccent),
          oscBLevelKnob(apvts, "oscBLevel", "Level", SDFLookAndFeel::secondaryAccent),
          oscBSemiKnob(apvts, "oscBSemitone", "Semi", SDFLookAndFeel::secondaryAccent),
          oscBFineKnob(apvts, "oscBFine", "Fine", SDFLookAndFeel::secondaryAccent),
          oscBFMKnob(apvts, "oscBFMDepth", "FM", SDFLookAndFeel::secondaryAccent),
          noiseLevelKnob(apvts, "noiseLevel", "Level", SDFLookAndFeel::tertiaryAccent),
          noiseFilterKnob(apvts, "noiseFilterCutoff", "Filter", SDFLookAndFeel::tertiaryAccent)
    {
        addAndMakeVisible(oscFoldKnob);
        addAndMakeVisible(oscPDKnob);
        addAndMakeVisible(oscPWKnob);
        addAndMakeVisible(oscSyncKnob);
        addAndMakeVisible(uniVoicesKnob);
        addAndMakeVisible(uniDetuneKnob);
        addAndMakeVisible(uniSpreadKnob);
        addAndMakeVisible(uniBlendKnob);

        // Osc B
        oscBEnableBtn.setColour(juce::ToggleButton::tickColourId, SDFLookAndFeel::secondaryAccent);
        oscBEnableBtn.setColour(juce::ToggleButton::tickDisabledColourId, SDFLookAndFeel::mutedText);
        addAndMakeVisible(oscBEnableBtn);
        oscBEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "oscBEnable", oscBEnableBtn);

        addAndMakeVisible(oscBLevelKnob);
        addAndMakeVisible(oscBSemiKnob);
        addAndMakeVisible(oscBFineKnob);
        addAndMakeVisible(oscBFMKnob);

        oscBMixSelector.addItem("Add", 1);
        oscBMixSelector.addItem("Ring", 2);
        oscBMixSelector.addItem("FM", 3);
        oscBMixSelector.addItem("AM", 4);
        int initMix = static_cast<int>(apvts.getRawParameterValue("oscBMixMode")->load());
        oscBMixSelector.setSelectedId(initMix + 1, juce::dontSendNotification);
        oscBMixSelector.onChange = [this]()
        {
            int id = oscBMixSelector.getSelectedId();
            if (id > 0)
                if (auto* p = apvtsRef.getParameter("oscBMixMode"))
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(id - 1)));
        };
        oscBMixSelector.setColour(juce::ComboBox::backgroundColourId, SDFLookAndFeel::panelBg);
        oscBMixSelector.setColour(juce::ComboBox::textColourId, SDFLookAndFeel::secondaryAccent);
        oscBMixSelector.setColour(juce::ComboBox::outlineColourId, SDFLookAndFeel::borderColour);
        addAndMakeVisible(oscBMixSelector);

        // Noise
        noiseEnableBtn.setColour(juce::ToggleButton::tickColourId, SDFLookAndFeel::tertiaryAccent);
        noiseEnableBtn.setColour(juce::ToggleButton::tickDisabledColourId, SDFLookAndFeel::mutedText);
        addAndMakeVisible(noiseEnableBtn);
        noiseEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "noiseEnable", noiseEnableBtn);
        addAndMakeVisible(noiseLevelKnob);
        addAndMakeVisible(noiseFilterKnob);

        noiseTypeSelector.addItem("White", 1);
        noiseTypeSelector.addItem("Pink", 2);
        noiseTypeSelector.addItem("Brown", 3);
        int initNoise = static_cast<int>(apvts.getRawParameterValue("noiseType")->load());
        noiseTypeSelector.setSelectedId(initNoise + 1, juce::dontSendNotification);
        noiseTypeSelector.onChange = [this]()
        {
            int id = noiseTypeSelector.getSelectedId();
            if (id > 0)
                if (auto* p = apvtsRef.getParameter("noiseType"))
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(id - 1)));
        };
        noiseTypeSelector.setColour(juce::ComboBox::backgroundColourId, SDFLookAndFeel::panelBg);
        noiseTypeSelector.setColour(juce::ComboBox::textColourId, SDFLookAndFeel::tertiaryAccent);
        noiseTypeSelector.setColour(juce::ComboBox::outlineColourId, SDFLookAndFeel::borderColour);
        addAndMakeVisible(noiseTypeSelector);

        oversampleSelector.addItem("OS Off", 1);
        oversampleSelector.addItem("2x", 2);
        oversampleSelector.addItem("4x", 3);
        int initOS = static_cast<int>(apvts.getRawParameterValue("oversample")->load());
        oversampleSelector.setSelectedId(initOS + 1, juce::dontSendNotification);
        oversampleSelector.onChange = [this]()
        {
            int id = oversampleSelector.getSelectedId();
            if (id > 0)
                if (auto* p = apvtsRef.getParameter("oversample"))
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(id - 1)));
        };
        oversampleSelector.setColour(juce::ComboBox::backgroundColourId, SDFLookAndFeel::panelBg);
        oversampleSelector.setColour(juce::ComboBox::textColourId, SDFLookAndFeel::tertiaryAccent);
        oversampleSelector.setColour(juce::ComboBox::outlineColourId, SDFLookAndFeel::borderColour);
        addAndMakeVisible(oversampleSelector);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 2);

        // Row 1: Osc effects (4 knobs) + Unison (4 knobs)
        int rowH = juce::jmax(1, bounds.getHeight() / 3);
        {
            auto row = bounds.removeFromTop(rowH);
            int kw = row.getWidth() / 8;
            oscFoldKnob.setBounds(row.removeFromLeft(kw));
            oscPDKnob.setBounds(row.removeFromLeft(kw));
            oscPWKnob.setBounds(row.removeFromLeft(kw));
            oscSyncKnob.setBounds(row.removeFromLeft(kw));
            uniVoicesKnob.setBounds(row.removeFromLeft(kw));
            uniDetuneKnob.setBounds(row.removeFromLeft(kw));
            uniSpreadKnob.setBounds(row.removeFromLeft(kw));
            uniBlendKnob.setBounds(row);
        }

        // Row 2: Osc B
        {
            auto row = bounds.removeFromTop(rowH);
            oscBEnableBtn.setBounds(row.removeFromLeft(24).reduced(0, 2));
            int bkw = juce::jmax(1, (row.getWidth() - 50) / 4);
            oscBLevelKnob.setBounds(row.removeFromLeft(bkw));
            oscBSemiKnob.setBounds(row.removeFromLeft(bkw));
            oscBFineKnob.setBounds(row.removeFromLeft(bkw));
            oscBMixSelector.setBounds(row.removeFromLeft(50).reduced(0, juce::jmax(0, (row.getHeight() - 20) / 2)));
            oscBFMKnob.setBounds(row);
        }

        // Row 3: Noise + Oversampling
        {
            auto row = bounds;
            noiseEnableBtn.setBounds(row.removeFromLeft(24).reduced(0, 2));
            int nkw = juce::jmax(1, (row.getWidth() - 110) / 2);
            noiseLevelKnob.setBounds(row.removeFromLeft(nkw));
            noiseTypeSelector.setBounds(row.removeFromLeft(50).reduced(0, juce::jmax(0, (row.getHeight() - 20) / 2)));
            noiseFilterKnob.setBounds(row.removeFromLeft(nkw));
            oversampleSelector.setBounds(row.reduced(2, juce::jmax(0, (row.getHeight() - 20) / 2)));
        }
    }

    void paint(juce::Graphics& g) override
    {
        SDFLookAndFeel::drawPanel(g, getLocalBounds(), "OSCILLATOR", SDFLookAndFeel::secondaryAccent);
    }

    // Accessors for mod routing
    ArcKnob& getOscFoldKnob() { return oscFoldKnob; }
    ArcKnob& getOscPDKnob() { return oscPDKnob; }
    ArcKnob& getOscPWKnob() { return oscPWKnob; }
    ArcKnob& getOscSyncKnob() { return oscSyncKnob; }
    ArcKnob& getUniDetuneKnob() { return uniDetuneKnob; }
    ArcKnob& getUniSpreadKnob() { return uniSpreadKnob; }
    ArcKnob& getOscBLevelKnob() { return oscBLevelKnob; }
    ArcKnob& getOscBFMKnob() { return oscBFMKnob; }
    ArcKnob& getNoiseLevelKnob() { return noiseLevelKnob; }
    ArcKnob& getNoiseFilterKnob() { return noiseFilterKnob; }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    ArcKnob oscFoldKnob, oscPDKnob, oscPWKnob, oscSyncKnob;
    ArcKnob uniVoicesKnob, uniDetuneKnob, uniSpreadKnob, uniBlendKnob;
    juce::ToggleButton oscBEnableBtn{ "B" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oscBEnableAttach;
    ArcKnob oscBLevelKnob, oscBSemiKnob, oscBFineKnob, oscBFMKnob;
    juce::ComboBox oscBMixSelector;
    juce::ToggleButton noiseEnableBtn{ "N" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> noiseEnableAttach;
    ArcKnob noiseLevelKnob, noiseFilterKnob;
    juce::ComboBox noiseTypeSelector;
    juce::ComboBox oversampleSelector;
};
