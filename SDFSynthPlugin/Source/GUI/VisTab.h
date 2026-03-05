#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SDFLookAndFeel.h"
#include "ArcKnob.h"
#include "TextureMenu.h"
#include "SkyboxSystem.h"

class VisTab : public juce::Component
{
public:
    VisTab(juce::AudioProcessorValueTreeState& apvts, TextureSystem& texSys, SDFViewport3D& vp)
        : textureMenu(apvts, texSys),
          skyExpKnob(apvts, "skyboxExposure", "Expose", SDFLookAndFeel::primaryAccent),
          skyRotKnob(apvts, "skyboxRotation", "Rotate", SDFLookAndFeel::primaryAccent),
          skyReflKnob(apvts, "skyboxReflect", "Reflect", SDFLookAndFeel::primaryAccent),
          skyBlurKnob(apvts, "skyboxBlur", "Blur", SDFLookAndFeel::primaryAccent),
          viewport(vp)
    {
        addAndMakeVisible(textureMenu);

        auto names = SkyboxSystem::getPresetNames();
        skyboxSelector.addItem("None", 1);
        for (int i = 0; i < names.size(); ++i)
            skyboxSelector.addItem(names[i], i + 2);
        skyboxSelector.setSelectedId(1, juce::dontSendNotification);
        skyboxSelector.onChange = [this]()
        {
            int id = skyboxSelector.getSelectedId();
            if (id <= 1)
                viewport.skyboxSystem.clear();
            else
                viewport.skyboxSystem.loadPreset(id - 2);
        };
        skyboxSelector.setColour(juce::ComboBox::backgroundColourId, SDFLookAndFeel::panelBg);
        skyboxSelector.setColour(juce::ComboBox::textColourId, SDFLookAndFeel::primaryAccent);
        skyboxSelector.setColour(juce::ComboBox::outlineColourId, SDFLookAndFeel::borderColour);
        addAndMakeVisible(skyboxSelector);

        skyLoadBtn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
        skyLoadBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
        skyLoadBtn.onClick = [this]()
        {
            auto chooser = std::make_shared<juce::FileChooser>(
                "Load HDR skybox", juce::File{}, "*.hdr");
            chooser->launchAsync(juce::FileBrowserComponent::openMode
                               | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser](const juce::FileChooser& fc)
                {
                    auto file = fc.getResult();
                    if (file.existsAsFile())
                    {
                        if (viewport.skyboxSystem.loadHDR(file))
                            skyboxSelector.setSelectedId(1, juce::dontSendNotification);
                    }
                });
        };
        addAndMakeVisible(skyLoadBtn);

        addAndMakeVisible(skyExpKnob);
        addAndMakeVisible(skyRotKnob);
        addAndMakeVisible(skyReflKnob);
        addAndMakeVisible(skyBlurKnob);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 2);
        int pad = 4;

        // Split: 60% texture, 40% skybox
        int texW = static_cast<int>(bounds.getWidth() * 0.60f);
        auto texSide = bounds.removeFromLeft(texW);
        bounds.removeFromLeft(pad);
        auto skySide = bounds;

        textureMenu.setBounds(texSide);

        auto skySelRow = skySide.removeFromTop(20);
        skyLoadBtn.setBounds(skySelRow.removeFromRight(32));
        skySelRow.removeFromRight(4);
        skyboxSelector.setBounds(skySelRow);
        skySide.removeFromTop(4);

        int skyKW = skySide.getWidth() / 2;
        int skyKH = juce::jmax(1, skySide.getHeight() / 2);
        auto sr1 = skySide.removeFromTop(skyKH);
        skyExpKnob.setBounds(sr1.removeFromLeft(skyKW));
        skyRotKnob.setBounds(sr1);
        auto sr2 = skySide;
        skyReflKnob.setBounds(sr2.removeFromLeft(skyKW));
        skyBlurKnob.setBounds(sr2);
    }

    void paint(juce::Graphics& g) override
    {
        SDFLookAndFeel::drawPanel(g, getLocalBounds(), "VISUAL", SDFLookAndFeel::primaryAccent);
    }

    TextureMenu& getTextureMenu() { return textureMenu; }
    juce::ComboBox& getSkyboxSelector() { return skyboxSelector; }

private:
    TextureMenu textureMenu;
    juce::ComboBox skyboxSelector;
    juce::TextButton skyLoadBtn{ "HDR" };
    ArcKnob skyExpKnob, skyRotKnob, skyReflKnob, skyBlurKnob;
    SDFViewport3D& viewport;
};
