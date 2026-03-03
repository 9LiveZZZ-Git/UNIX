#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SDFLookAndFeel.h"
#include "ArcKnob.h"
#include "Utility/Constants.h"

class ModTab : public juce::Component
{
public:
    ModTab(juce::AudioProcessorValueTreeState& apvts)
        : lfo1RateKnob(apvts, "lfo1Rate", "Rate", SDFLookAndFeel::primaryAccent),
          lfo1PhaseKnob(apvts, "lfo1Phase", "Phase", SDFLookAndFeel::primaryAccent),
          lfo2RateKnob(apvts, "lfo2Rate", "Rate", SDFLookAndFeel::secondaryAccent),
          lfo2PhaseKnob(apvts, "lfo2Phase", "Phase", SDFLookAndFeel::secondaryAccent)
    {
        addAndMakeVisible(lfo1RateKnob);
        addAndMakeVisible(lfo1PhaseKnob);
        addAndMakeVisible(lfo2RateKnob);
        addAndMakeVisible(lfo2PhaseKnob);

        // LFO shapes
        auto lfoShapeNames = juce::StringArray{ "Sine", "Tri", "Saw", "Sqr", "S&H", "Rnd" };
        auto setupLfoShape = [&](juce::ComboBox& sel, const juce::String& paramId, juce::Colour col)
        {
            for (int i = 0; i < lfoShapeNames.size(); ++i)
                sel.addItem(lfoShapeNames[i], i + 1);
            int init = static_cast<int>(apvts.getRawParameterValue(paramId)->load());
            sel.setSelectedId(init + 1, juce::dontSendNotification);
            sel.onChange = [&sel, &apvts, paramId]()
            {
                int id = sel.getSelectedId();
                if (id > 0)
                    if (auto* p = apvts.getParameter(paramId))
                        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(id - 1)));
            };
            sel.setColour(juce::ComboBox::backgroundColourId, SDFLookAndFeel::panelBg);
            sel.setColour(juce::ComboBox::textColourId, col);
            sel.setColour(juce::ComboBox::outlineColourId, SDFLookAndFeel::borderColour);
            addAndMakeVisible(sel);
        };

        setupLfoShape(lfo1ShapeSelector, "lfo1Shape", SDFLookAndFeel::primaryAccent);
        setupLfoShape(lfo2ShapeSelector, "lfo2Shape", SDFLookAndFeel::secondaryAccent);

        // LFO Sync toggles
        lfo1SyncBtn.setColour(juce::ToggleButton::tickColourId, SDFLookAndFeel::primaryAccent);
        lfo1SyncBtn.setColour(juce::ToggleButton::tickDisabledColourId, SDFLookAndFeel::mutedText);
        addAndMakeVisible(lfo1SyncBtn);
        lfo1SyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "lfo1Sync", lfo1SyncBtn);

        lfo2SyncBtn.setColour(juce::ToggleButton::tickColourId, SDFLookAndFeel::secondaryAccent);
        lfo2SyncBtn.setColour(juce::ToggleButton::tickDisabledColourId, SDFLookAndFeel::mutedText);
        addAndMakeVisible(lfo2SyncBtn);
        lfo2SyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "lfo2Sync", lfo2SyncBtn);

        // Source drag handles
        struct SrcInfo { const char* label; juce::Colour colour; };
        SrcInfo srcs[] = {
            {"ENV", juce::Colour(sdfColour::modEnvelope)},
            {"L1",  juce::Colour(sdfColour::modLFO1)},
            {"L2",  juce::Colour(sdfColour::modLFO2)},
            {"VEL", juce::Colour(sdfColour::modVelocity)},
            {"KEY", juce::Colour(sdfColour::modKeyTrack)},
            {"AT",  juce::Colour(sdfColour::modAftertouch)},
            {"MW",  juce::Colour(sdfColour::modModWheel)},
            {"RND", juce::Colour(sdfColour::modRandom)},
            {"M1",  juce::Colour(sdfColour::modMacro)},
            {"M2",  juce::Colour(sdfColour::modMacro)},
            {"M3",  juce::Colour(sdfColour::modMacro)},
            {"M4",  juce::Colour(sdfColour::modMacro)},
        };
        for (int i = 0; i < 12; ++i)
        {
            auto* lbl = modSourceHandles.add(new juce::Label());
            lbl->setText(srcs[i].label, juce::dontSendNotification);
            lbl->setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(9.f), juce::Font::bold));
            lbl->setColour(juce::Label::textColourId, srcs[i].colour);
            lbl->setColour(juce::Label::backgroundColourId, SDFLookAndFeel::panelBg);
            lbl->setColour(juce::Label::outlineColourId, SDFLookAndFeel::borderColour);
            lbl->setJustificationType(juce::Justification::centred);
            lbl->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(lbl);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 2);

        // Row 1: LFO 1 | LFO 2
        int rowH = static_cast<int>(bounds.getHeight() * 0.55f);
        auto row1 = bounds.removeFromTop(rowH);
        int halfW = row1.getWidth() / 2;

        // LFO 1
        {
            auto area = row1.removeFromLeft(halfW);
            int kw = juce::jmax(1, (area.getWidth() - 70) / 2);
            lfo1RateKnob.setBounds(area.removeFromLeft(kw));
            lfo1ShapeSelector.setBounds(area.removeFromLeft(50).reduced(0, juce::jmax(0, (area.getHeight() - 20) / 2)));
            lfo1SyncBtn.setBounds(area.removeFromLeft(20).reduced(0, 2));
            lfo1PhaseKnob.setBounds(area);
        }

        // LFO 2
        {
            auto area = row1;
            int kw = juce::jmax(1, (area.getWidth() - 70) / 2);
            lfo2RateKnob.setBounds(area.removeFromLeft(kw));
            lfo2ShapeSelector.setBounds(area.removeFromLeft(50).reduced(0, juce::jmax(0, (area.getHeight() - 20) / 2)));
            lfo2SyncBtn.setBounds(area.removeFromLeft(20).reduced(0, 2));
            lfo2PhaseKnob.setBounds(area);
        }

        // Row 2: Source handles (6x2 grid)
        auto handleArea = bounds.reduced(4, 2);
        int handleW = handleArea.getWidth() / 6;
        int handleH = handleArea.getHeight() / 2;
        for (int i = 0; i < modSourceHandles.size() && i < 12; ++i)
        {
            int r = i / 6;
            int c = i % 6;
            modSourceHandles[i]->setBounds(
                handleArea.getX() + c * handleW + 1,
                handleArea.getY() + r * handleH + 1,
                handleW - 2, handleH - 2);
        }
    }

    void paint(juce::Graphics& g) override
    {
        SDFLookAndFeel::drawPanel(g, getLocalBounds(), "MODULATION", SDFLookAndFeel::primaryAccent);
    }

    juce::OwnedArray<juce::Label>& getModSourceHandles() { return modSourceHandles; }

private:
    ArcKnob lfo1RateKnob, lfo1PhaseKnob;
    ArcKnob lfo2RateKnob, lfo2PhaseKnob;
    juce::ComboBox lfo1ShapeSelector, lfo2ShapeSelector;
    juce::ToggleButton lfo1SyncBtn{ "S" }, lfo2SyncBtn{ "S" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lfo1SyncAttach, lfo2SyncAttach;
    juce::OwnedArray<juce::Label> modSourceHandles;
};
