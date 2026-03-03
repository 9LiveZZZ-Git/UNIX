#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SDFLookAndFeel.h"
#include "ArcKnob.h"
#include "ShapeSelector.h"
#include "SegmentedControl.h"

class ShapeTab : public juce::Component
{
public:
    ShapeTab(juce::AudioProcessorValueTreeState& apvts)
        : shapeASelector(apvts, "shape1", "A"),
          shapeBSelector(apvts, "shape2", "B"),
          sizeAKnob(apvts, "size1", "Size A", SDFLookAndFeel::primaryAccent),
          sizeBKnob(apvts, "size2", "Size B", SDFLookAndFeel::primaryAccent),
          offsetXKnob(apvts, "offsetX", "Off X", SDFLookAndFeel::primaryAccent),
          offsetYKnob(apvts, "offsetY", "Off Y", SDFLookAndFeel::primaryAccent),
          smoothKKnob(apvts, "smoothK", "Smooth", SDFLookAndFeel::primaryAccent),
          twistKnob(apvts, "twist", "Twist", SDFLookAndFeel::primaryAccent),
          scanRadiusKnob(apvts, "scanRadius", "Radius", SDFLookAndFeel::secondaryAccent),
          scanHeightKnob(apvts, "scanHeight", "Height", SDFLookAndFeel::secondaryAccent),
          topoMorphKnob(apvts, "topoMorph", "MRI", SDFLookAndFeel::secondaryAccent),
          distScaleKnob(apvts, "distScale", "Scale", SDFLookAndFeel::secondaryAccent),
          sfMKnob(apvts, "sfM", "SF M", SDFLookAndFeel::primaryAccent),
          sfN1Knob(apvts, "sfN1", "SF N1", SDFLookAndFeel::primaryAccent),
          sfN2Knob(apvts, "sfN2", "SF N2", SDFLookAndFeel::primaryAccent),
          sfN3Knob(apvts, "sfN3", "SF N3", SDFLookAndFeel::primaryAccent),
          onionThicknessKnob(apvts, "onionThickness", "Shell", SDFLookAndFeel::primaryAccent),
          stairCountKnob(apvts, "stairCount", "Steps", SDFLookAndFeel::primaryAccent),
          apvtsRef(apvts)
    {
        addAndMakeVisible(shapeASelector);
        addAndMakeVisible(shapeBSelector);

        // Operation buttons (11 ops)
        juce::StringArray opNames = { "SMTH", "UNION", "INTER", "SUB",
                                       "SM.I", "SM.S", "CH.U", "CH.I", "CH.S", "STRS", "PIPE" };
        for (int i = 0; i < opNames.size(); ++i)
        {
            auto* btn = opButtons.add(new juce::TextButton(opNames[i]));
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(9999);
            btn->setColour(juce::TextButton::buttonOnColourId, SDFLookAndFeel::secondaryAccent);
            btn->setColour(juce::TextButton::textColourOnId, SDFLookAndFeel::secondaryAccent);
            btn->setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
            btn->onClick = [this, i]()
            {
                if (auto* param = apvtsRef.getParameter("operation"))
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
            };
            addAndMakeVisible(btn);
        }

        int initOp = static_cast<int>(apvts.getRawParameterValue("operation")->load());
        if (initOp >= 0 && initOp < opButtons.size())
            opButtons[initOp]->setToggleState(true, juce::dontSendNotification);

        // Scan mode buttons
        juce::StringArray modeNames = { "CONTOUR", "MARCH", "ACOUSTIC", "GRAIN", "SPECTRAL", "TRAVERSE" };
        for (int i = 0; i < modeNames.size(); ++i)
        {
            auto* btn = scanModeButtons.add(new juce::TextButton(modeNames[i]));
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(8888);
            btn->setColour(juce::TextButton::buttonOnColourId, SDFLookAndFeel::secondaryAccent);
            btn->setColour(juce::TextButton::textColourOnId, SDFLookAndFeel::secondaryAccent);
            btn->setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
            btn->onClick = [this, i]()
            {
                if (auto* param = apvtsRef.getParameter("scanMode"))
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
            };
            addAndMakeVisible(btn);
        }

        int initMode = static_cast<int>(apvts.getRawParameterValue("scanMode")->load());
        if (initMode >= 0 && initMode < scanModeButtons.size())
            scanModeButtons[initMode]->setToggleState(true, juce::dontSendNotification);

        // Onion toggle
        onionEnableBtn.setColour(juce::ToggleButton::tickColourId, SDFLookAndFeel::primaryAccent);
        onionEnableBtn.setColour(juce::ToggleButton::tickDisabledColourId, SDFLookAndFeel::mutedText);
        addAndMakeVisible(onionEnableBtn);
        onionEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "onionEnable", onionEnableBtn);

        addAndMakeVisible(sizeAKnob);
        addAndMakeVisible(sizeBKnob);
        addAndMakeVisible(offsetXKnob);
        addAndMakeVisible(offsetYKnob);
        addAndMakeVisible(smoothKKnob);
        addAndMakeVisible(twistKnob);
        addAndMakeVisible(scanRadiusKnob);
        addAndMakeVisible(scanHeightKnob);
        addAndMakeVisible(topoMorphKnob);
        addAndMakeVisible(distScaleKnob);
        addAndMakeVisible(sfMKnob);
        addAndMakeVisible(sfN1Knob);
        addAndMakeVisible(sfN2Knob);
        addAndMakeVisible(sfN3Knob);
        addAndMakeVisible(onionThicknessKnob);
        addAndMakeVisible(stairCountKnob);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 2);
        int pad = 2;

        // Shape selectors (2 rows of 24px)
        shapeASelector.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(pad);

        // Operation buttons: 2 rows of 6
        {
            int cols = 6;
            int opRows = (opButtons.size() + cols - 1) / cols;
            for (int r = 0; r < opRows; ++r)
            {
                auto row = bounds.removeFromTop(18);
                int start = r * cols;
                int end = juce::jmin(start + cols, opButtons.size());
                int count = end - start;
                int bw = row.getWidth() / count;
                for (int i = start; i < end; ++i)
                    opButtons[i]->setBounds(row.removeFromLeft(bw));
            }
        }
        bounds.removeFromTop(pad);

        shapeBSelector.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(pad);

        // Scene knobs: 2 rows of 3
        {
            int kw = bounds.getWidth() / 3;
            int kh = juce::jmax(1, static_cast<int>(bounds.getHeight() * 0.18f));
            auto r1 = bounds.removeFromTop(kh);
            sizeAKnob.setBounds(r1.removeFromLeft(kw));
            sizeBKnob.setBounds(r1.removeFromLeft(kw));
            offsetXKnob.setBounds(r1);
            auto r2 = bounds.removeFromTop(kh);
            offsetYKnob.setBounds(r2.removeFromLeft(kw));
            smoothKKnob.setBounds(r2.removeFromLeft(kw));
            twistKnob.setBounds(r2);
        }
        bounds.removeFromTop(pad);

        // Scan mode buttons: 2 rows of 3
        {
            auto sr1 = bounds.removeFromTop(18);
            auto sr2 = bounds.removeFromTop(18);
            int bw = sr1.getWidth() / 3;
            for (int i = 0; i < scanModeButtons.size(); ++i)
            {
                auto& row = (i < 3) ? sr1 : sr2;
                scanModeButtons[i]->setBounds(row.removeFromLeft(bw));
            }
        }
        bounds.removeFromTop(pad);

        // Scan knobs: 1 row of 4
        {
            int kw = bounds.getWidth() / 4;
            int kh = juce::jmax(1, static_cast<int>(bounds.getHeight() * 0.35f));
            auto row = bounds.removeFromTop(kh);
            scanRadiusKnob.setBounds(row.removeFromLeft(kw));
            scanHeightKnob.setBounds(row.removeFromLeft(kw));
            topoMorphKnob.setBounds(row.removeFromLeft(kw));
            distScaleKnob.setBounds(row);
        }
        bounds.removeFromTop(pad);

        // Bottom: SuperFormula + Onion + StairCount
        {
            int kw = bounds.getWidth() / 6;
            auto row = bounds;
            sfMKnob.setBounds(row.removeFromLeft(kw));
            sfN1Knob.setBounds(row.removeFromLeft(kw));
            sfN2Knob.setBounds(row.removeFromLeft(kw));
            sfN3Knob.setBounds(row.removeFromLeft(kw));
            onionEnableBtn.setBounds(row.removeFromLeft(24).reduced(0, 2));
            onionThicknessKnob.setBounds(row.removeFromLeft(kw - 12));
            stairCountKnob.setBounds(row);
        }
    }

    void paint(juce::Graphics& g) override
    {
        SDFLookAndFeel::drawPanel(g, getLocalBounds(), "SHAPE & SCAN", SDFLookAndFeel::primaryAccent);
    }

    // Accessors for mod routing
    ArcKnob& getSizeAKnob() { return sizeAKnob; }
    ArcKnob& getSizeBKnob() { return sizeBKnob; }
    ArcKnob& getOffsetXKnob() { return offsetXKnob; }
    ArcKnob& getOffsetYKnob() { return offsetYKnob; }
    ArcKnob& getSmoothKKnob() { return smoothKKnob; }
    ArcKnob& getTwistKnob() { return twistKnob; }
    ArcKnob& getScanRadiusKnob() { return scanRadiusKnob; }
    ArcKnob& getScanHeightKnob() { return scanHeightKnob; }
    ArcKnob& getTopoMorphKnob() { return topoMorphKnob; }
    ArcKnob& getDistScaleKnob() { return distScaleKnob; }
    juce::OwnedArray<juce::TextButton>& getOpButtons() { return opButtons; }
    juce::OwnedArray<juce::TextButton>& getScanModeButtons() { return scanModeButtons; }

private:
    ShapeSelector shapeASelector, shapeBSelector;
    juce::OwnedArray<juce::TextButton> opButtons;
    juce::OwnedArray<juce::TextButton> scanModeButtons;
    ArcKnob sizeAKnob, sizeBKnob, offsetXKnob, offsetYKnob, smoothKKnob, twistKnob;
    ArcKnob scanRadiusKnob, scanHeightKnob, topoMorphKnob, distScaleKnob;
    ArcKnob sfMKnob, sfN1Knob, sfN2Knob, sfN3Knob;
    ArcKnob onionThicknessKnob, stairCountKnob;
    juce::ToggleButton onionEnableBtn{ "Onion" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onionEnableAttach;
    juce::AudioProcessorValueTreeState& apvtsRef;
};
