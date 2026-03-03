#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SDFLookAndFeel.h"
#include "ArcKnob.h"
#include "ADSRDisplay.h"
#include "SegmentedControl.h"

class BottomStrip : public juce::Component
{
public:
    BottomStrip(juce::AudioProcessorValueTreeState& apvts)
        : filterCutKnob(apvts, "filterCutoff", "Filter", SDFLookAndFeel::tertiaryAccent),
          filterResKnob(apvts, "filterRes", "Reson", SDFLookAndFeel::tertiaryAccent),
          filterModeCtrl(apvts, "filterMode", { "LP", "BP", "HP", "Notch", "Peak" }, SDFLookAndFeel::tertiaryAccent),
          attackKnob(apvts, "attack", "Atk", SDFLookAndFeel::tertiaryAccent),
          decayKnob(apvts, "decay", "Dec", SDFLookAndFeel::tertiaryAccent),
          sustainKnob(apvts, "sustain", "Sus", SDFLookAndFeel::tertiaryAccent),
          releaseKnob(apvts, "release", "Rel", SDFLookAndFeel::tertiaryAccent),
          adsrDisplay(apvts),
          gainKnob(apvts, "masterGain", "Volume", SDFLookAndFeel::tertiaryAccent),
          macro1Knob(apvts, "macro1", "M1", SDFLookAndFeel::tertiaryAccent),
          macro2Knob(apvts, "macro2", "M2", SDFLookAndFeel::tertiaryAccent),
          macro3Knob(apvts, "macro3", "M3", SDFLookAndFeel::tertiaryAccent),
          macro4Knob(apvts, "macro4", "M4", SDFLookAndFeel::tertiaryAccent)
    {
        addAndMakeVisible(filterCutKnob);
        addAndMakeVisible(filterResKnob);
        addAndMakeVisible(filterModeCtrl);
        addAndMakeVisible(attackKnob);
        addAndMakeVisible(decayKnob);
        addAndMakeVisible(sustainKnob);
        addAndMakeVisible(releaseKnob);
        addAndMakeVisible(adsrDisplay);
        addAndMakeVisible(gainKnob);
        addAndMakeVisible(macro1Knob);
        addAndMakeVisible(macro2Knob);
        addAndMakeVisible(macro3Knob);
        addAndMakeVisible(macro4Knob);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 2);

        // Filter section: ~22%
        int filterW = static_cast<int>(bounds.getWidth() * 0.22f);
        auto filterArea = bounds.removeFromLeft(filterW);
        {
            auto fmRow = filterArea.removeFromTop(18);
            filterModeCtrl.setBounds(fmRow);
            filterArea.removeFromTop(2);
            int kw = filterArea.getWidth() / 2;
            filterCutKnob.setBounds(filterArea.removeFromLeft(kw));
            filterResKnob.setBounds(filterArea);
        }

        bounds.removeFromLeft(4);

        // ADSR section: ~32%
        int adsrW = static_cast<int>(bounds.getWidth() * 0.38f);
        auto adsrArea = bounds.removeFromLeft(adsrW);
        {
            int kw = adsrArea.getWidth() / 2;
            auto knobSide = adsrArea.removeFromLeft(kw);
            int akw = knobSide.getWidth() / 2;
            int akh = knobSide.getHeight() / 2;
            auto r1 = knobSide.removeFromTop(akh);
            attackKnob.setBounds(r1.removeFromLeft(akw));
            decayKnob.setBounds(r1);
            auto r2 = knobSide;
            sustainKnob.setBounds(r2.removeFromLeft(akw));
            releaseKnob.setBounds(r2);
            adsrDisplay.setBounds(adsrArea);
        }

        bounds.removeFromLeft(4);

        // Gain: ~8%
        int gainW = static_cast<int>(bounds.getWidth() * 0.12f);
        gainKnob.setBounds(bounds.removeFromLeft(gainW));

        bounds.removeFromLeft(4);

        // Macros: rest
        int mkw = bounds.getWidth() / 4;
        macro1Knob.setBounds(bounds.removeFromLeft(mkw));
        macro2Knob.setBounds(bounds.removeFromLeft(mkw));
        macro3Knob.setBounds(bounds.removeFromLeft(mkw));
        macro4Knob.setBounds(bounds);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(SDFLookAndFeel::panelBg);
        g.fillRoundedRectangle(bounds, SDFLookAndFeel::cornerRadius);
        g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, SDFLookAndFeel::cornerRadius, 1.f);
    }

    // Public accessors for mod routing
    ArcKnob& getFilterCutKnob() { return filterCutKnob; }
    ArcKnob& getFilterResKnob() { return filterResKnob; }
    ArcKnob& getAttackKnob() { return attackKnob; }
    ArcKnob& getDecayKnob() { return decayKnob; }
    ArcKnob& getSustainKnob() { return sustainKnob; }
    ArcKnob& getReleaseKnob() { return releaseKnob; }
    ArcKnob& getGainKnob() { return gainKnob; }
    ArcKnob& getMacro1Knob() { return macro1Knob; }
    ArcKnob& getMacro2Knob() { return macro2Knob; }
    ArcKnob& getMacro3Knob() { return macro3Knob; }
    ArcKnob& getMacro4Knob() { return macro4Knob; }
    ADSRDisplay& getAdsrDisplay() { return adsrDisplay; }

private:
    ArcKnob filterCutKnob, filterResKnob;
    SegmentedControl filterModeCtrl;
    ArcKnob attackKnob, decayKnob, sustainKnob, releaseKnob;
    ADSRDisplay adsrDisplay;
    ArcKnob gainKnob;
    ArcKnob macro1Knob, macro2Knob, macro3Knob, macro4Knob;
};
