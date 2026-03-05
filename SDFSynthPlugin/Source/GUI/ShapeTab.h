#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SDFLookAndFeel.h"
#include "ArcKnob.h"

class ShapeTab : public juce::Component
{
public:
    ShapeTab(juce::AudioProcessorValueTreeState& apvts)
        : sizeAKnob(apvts, "size1", "Size A", SDFLookAndFeel::primaryAccent),
          sizeBKnob(apvts, "size2", "Size B", SDFLookAndFeel::primaryAccent),
          offsetXKnob(apvts, "offsetX", "Off X", SDFLookAndFeel::primaryAccent),
          offsetYKnob(apvts, "offsetY", "Off Y", SDFLookAndFeel::primaryAccent),
          smoothKKnob(apvts, "smoothK", "Smooth", SDFLookAndFeel::primaryAccent),
          twistKnob(apvts, "twist", "Twist", SDFLookAndFeel::primaryAccent),
          scanRadiusKnob(apvts, "scanRadius", "Radius", SDFLookAndFeel::secondaryAccent),
          scanHeightKnob(apvts, "scanHeight", "Height", SDFLookAndFeel::secondaryAccent),
          topoMorphKnob(apvts, "topoMorph", "MRI", SDFLookAndFeel::secondaryAccent),
          distScaleKnob(apvts, "distScale", "Scale", SDFLookAndFeel::secondaryAccent),
          sfMKnob(apvts, "sfM", "SF M", SDFLookAndFeel::secondaryAccent),
          sfN1Knob(apvts, "sfN1", "SF N1", SDFLookAndFeel::secondaryAccent),
          sfN2Knob(apvts, "sfN2", "SF N2", SDFLookAndFeel::secondaryAccent),
          sfN3Knob(apvts, "sfN3", "SF N3", SDFLookAndFeel::secondaryAccent),
          onionThicknessKnob(apvts, "onionThickness", "Shell", SDFLookAndFeel::secondaryAccent),
          stairCountKnob(apvts, "stairCount", "Steps", SDFLookAndFeel::secondaryAccent)
    {
        // Shape A dropdown
        auto shapeNames = getShapeNames();
        for (int i = 0; i < shapeNames.size(); ++i)
            shapeABox.addItem(shapeNames[i], i + 1);
        setupComboBox(shapeABox, SDFLookAndFeel::primaryAccent);
        addAndMakeVisible(shapeABox);
        shapeAAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "shape1", shapeABox);

        // Operation dropdown
        juce::StringArray opNames = { "Smooth Union", "Union", "Intersection", "Subtraction",
                                       "Smooth Intersect", "Smooth Subtract",
                                       "Chamfer Union", "Chamfer Intersect", "Chamfer Subtract",
                                       "Stairs", "Pipe" };
        for (int i = 0; i < opNames.size(); ++i)
            operationBox.addItem(opNames[i], i + 1);
        setupComboBox(operationBox, SDFLookAndFeel::secondaryAccent);
        addAndMakeVisible(operationBox);
        operationAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "operation", operationBox);

        // Shape B dropdown
        for (int i = 0; i < shapeNames.size(); ++i)
            shapeBBox.addItem(shapeNames[i], i + 1);
        setupComboBox(shapeBBox, SDFLookAndFeel::primaryAccent);
        addAndMakeVisible(shapeBBox);
        shapeBAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "shape2", shapeBBox);

        // Scan mode dropdown
        scanModeBox.addItem("Contour", 1);
        scanModeBox.addItem("March", 2);
        scanModeBox.addItem("Acoustic", 3);
        scanModeBox.addItem("Grain", 4);
        scanModeBox.addItem("Spectral", 5);
        setupComboBox(scanModeBox, SDFLookAndFeel::secondaryAccent);
        addAndMakeVisible(scanModeBox);
        scanModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "scanMode", scanModeBox);

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

        // Section labels
        addAndMakeVisible(shapeALabel);
        addAndMakeVisible(operationLabel);
        addAndMakeVisible(shapeBLabel);
        addAndMakeVisible(scanLabel);
        setupLabel(shapeALabel, "SHAPE A");
        setupLabel(operationLabel, "OP");
        setupLabel(shapeBLabel, "SHAPE B");
        setupLabel(scanLabel, "SCAN");
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 2);
        bounds.removeFromTop(SDFLookAndFeel::scaledInt(18)); // panel header
        int pad = 2;
        int comboH = 20;
        int labelW = 48;

        // Shape A: label + combo
        {
            auto row = bounds.removeFromTop(comboH);
            shapeALabel.setBounds(row.removeFromLeft(labelW));
            shapeABox.setBounds(row);
        }
        bounds.removeFromTop(pad);

        // Operation: label + combo
        {
            auto row = bounds.removeFromTop(comboH);
            operationLabel.setBounds(row.removeFromLeft(labelW));
            operationBox.setBounds(row);
        }
        bounds.removeFromTop(pad);

        // Shape B: label + combo
        {
            auto row = bounds.removeFromTop(comboH);
            shapeBLabel.setBounds(row.removeFromLeft(labelW));
            shapeBBox.setBounds(row);
        }
        bounds.removeFromTop(pad);

        // Remaining height split into scene knobs, scan combo, scan knobs, SF row
        int remaining = bounds.getHeight();
        int knobRowH = juce::jmax(SDFLookAndFeel::scaledInt(48), (remaining - comboH - pad) / 4);

        // Row 1: Scene knobs (SizeA, SizeB, OffX)
        {
            int kw = bounds.getWidth() / 3;
            auto row = bounds.removeFromTop(knobRowH);
            sizeAKnob.setBounds(row.removeFromLeft(kw));
            sizeBKnob.setBounds(row.removeFromLeft(kw));
            offsetXKnob.setBounds(row);
        }

        // Row 2: Scene knobs (OffY, Smooth, Twist)
        {
            int kw = bounds.getWidth() / 3;
            auto row = bounds.removeFromTop(knobRowH);
            offsetYKnob.setBounds(row.removeFromLeft(kw));
            smoothKKnob.setBounds(row.removeFromLeft(kw));
            twistKnob.setBounds(row);
        }

        // Scan mode: label + combo (in scan section)
        {
            auto row = bounds.removeFromTop(comboH);
            scanLabel.setBounds(row.removeFromLeft(labelW));
            scanModeBox.setBounds(row);
        }
        bounds.removeFromTop(pad);

        // Row 3: Scan knobs (Radius, Height, MRI, Scale)
        {
            int kw = bounds.getWidth() / 4;
            auto row = bounds.removeFromTop(knobRowH);
            scanRadiusKnob.setBounds(row.removeFromLeft(kw));
            scanHeightKnob.setBounds(row.removeFromLeft(kw));
            topoMorphKnob.setBounds(row.removeFromLeft(kw));
            distScaleKnob.setBounds(row);
        }

        // Row 4: SuperFormula + Onion + StairCount (takes remaining)
        {
            int kw = bounds.getWidth() / 6;
            auto row = bounds;
            sfMKnob.setBounds(row.removeFromLeft(kw));
            sfN1Knob.setBounds(row.removeFromLeft(kw));
            sfN2Knob.setBounds(row.removeFromLeft(kw));
            sfN3Knob.setBounds(row.removeFromLeft(kw));
            onionEnableBtn.setBounds(row.removeFromLeft(22).reduced(0, 2));
            onionThicknessKnob.setBounds(row.removeFromLeft(juce::jmax(0, kw - 11)));
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
    juce::ComboBox& getScanModeBox() { return scanModeBox; }

private:
    static juce::StringArray getShapeNames()
    {
        return { "Sphere", "Box", "Torus", "Cylinder", "Octahedron", "Custom OBJ",
                 "Capsule", "Round Box", "Hex Prism", "Torus 8/2", "Torus 8/8", "SuperFormula" };
    }

    static void setupComboBox(juce::ComboBox& box, juce::Colour textCol)
    {
        box.setColour(juce::ComboBox::backgroundColourId, SDFLookAndFeel::panelBg);
        box.setColour(juce::ComboBox::textColourId, textCol);
        box.setColour(juce::ComboBox::outlineColourId, SDFLookAndFeel::borderColour);
    }

    static void setupLabel(juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), SDFLookAndFeel::scaled(9.f), juce::Font::bold));
        label.setColour(juce::Label::textColourId, SDFLookAndFeel::mutedText);
        label.setJustificationType(juce::Justification::centredRight);
    }

    juce::ComboBox shapeABox, shapeBBox, operationBox, scanModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAAttach, shapeBAttach, operationAttach, scanModeAttach;
    juce::Label shapeALabel, operationLabel, shapeBLabel, scanLabel;
    ArcKnob sizeAKnob, sizeBKnob, offsetXKnob, offsetYKnob, smoothKKnob, twistKnob;
    ArcKnob scanRadiusKnob, scanHeightKnob, topoMorphKnob, distScaleKnob;
    ArcKnob sfMKnob, sfN1Knob, sfN2Knob, sfN3Knob;
    ArcKnob onionThicknessKnob, stairCountKnob;
    juce::ToggleButton onionEnableBtn{ "Onion" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onionEnableAttach;
};
