#include "PluginEditor.h"

SDFSynthEditor::SDFSynthEditor(SDFSynthProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      presetManager(p.apvts),
      viewport3D(p.apvts),
      textureMenu(p.apvts, viewport3D.textureSystem),
      shapeASelector(p.apvts, "shape1", "SHAPE A"),
      shapeBSelector(p.apvts, "shape2", "SHAPE B"),
      sizeAKnob(p.apvts, "size1", "Size A", SDFLookAndFeel::primaryAccent),
      sizeBKnob(p.apvts, "size2", "Size B", SDFLookAndFeel::primaryAccent),
      offsetXKnob(p.apvts, "offsetX", "Offset X", SDFLookAndFeel::primaryAccent),
      offsetYKnob(p.apvts, "offsetY", "Offset Y", SDFLookAndFeel::primaryAccent),
      smoothKKnob(p.apvts, "smoothK", "Smooth", SDFLookAndFeel::primaryAccent),
      twistKnob(p.apvts, "twist", "Twist", SDFLookAndFeel::primaryAccent),
      scanRadiusKnob(p.apvts, "scanRadius", "Radius", SDFLookAndFeel::secondaryAccent),
      scanHeightKnob(p.apvts, "scanHeight", "Height", SDFLookAndFeel::secondaryAccent),
      topoMorphKnob(p.apvts, "topoMorph", "MRI", SDFLookAndFeel::secondaryAccent),
      distScaleKnob(p.apvts, "distScale", "Scale", SDFLookAndFeel::secondaryAccent),
      filterCutKnob(p.apvts, "filterCutoff", "Filter", SDFLookAndFeel::tertiaryAccent),
      filterResKnob(p.apvts, "filterRes", "Reson", SDFLookAndFeel::tertiaryAccent),
      attackKnob(p.apvts, "attack", "Atk", SDFLookAndFeel::tertiaryAccent),
      decayKnob(p.apvts, "decay", "Dec", SDFLookAndFeel::tertiaryAccent),
      sustainKnob(p.apvts, "sustain", "Sus", SDFLookAndFeel::tertiaryAccent),
      releaseKnob(p.apvts, "release", "Rel", SDFLookAndFeel::tertiaryAccent),
      gainKnob(p.apvts, "masterGain", "Volume", SDFLookAndFeel::tertiaryAccent),
      adsrDisplay(p.apvts),
      skyExpKnob(p.apvts, "skyboxExposure", "Expose", SDFLookAndFeel::primaryAccent),
      skyRotKnob(p.apvts, "skyboxRotation", "Rotate", SDFLookAndFeel::primaryAccent),
      skyReflKnob(p.apvts, "skyboxReflect", "Reflect", SDFLookAndFeel::primaryAccent),
      skyBlurKnob(p.apvts, "skyboxBlur", "Blur", SDFLookAndFeel::primaryAccent),
      midiKeyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&lookAndFeel);

    // Wire texture system to processor for audio modulation
    processor.setTextureSystem(&viewport3D.textureSystem);
    textureMenu.onTextureChanged = [this]() { processor.markWavetableDirty(); };

    // Wire modulated param values to viewport for live visualization
    viewport3D.getModulatedValue = [this](const char* paramId) -> float {
        return processor.getModulatedParamValue(juce::String(paramId));
    };

    // Preset controls
    auto factoryNames = PresetManager::getFactoryPresetNames();
    presetSelector.addItem("-- Preset --", 1);
    for (int i = 0; i < factoryNames.size(); ++i)
        presetSelector.addItem(factoryNames[i], i + 2);
    presetSelector.setSelectedId(1, juce::dontSendNotification);
    presetSelector.onChange = [this]()
    {
        int id = presetSelector.getSelectedId();
        if (id > 1)
        {
            presetManager.loadFactoryPreset(id - 2);
            applyPresetResources();
        }
    };
    addAndMakeVisible(presetSelector);

    presetSaveBtn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
    presetSaveBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
    presetSaveBtn.onClick = [this]()
    {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Save preset", presetManager.getPresetsFolder(), "*.xml");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file != juce::File{})
                    presetManager.savePreset(file.withFileExtension("xml"));
            });
    };
    addAndMakeVisible(presetSaveBtn);

    presetLoadBtn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
    presetLoadBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
    presetLoadBtn.onClick = [this]()
    {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Load preset", presetManager.getPresetsFolder(), "*.xml");
        chooser->launchAsync(juce::FileBrowserComponent::openMode
                           | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                {
                    presetManager.loadPreset(file);
                    applyPresetResources();
                }
            });
    };
    addAndMakeVisible(presetLoadBtn);

    addAndMakeVisible(viewport3D);
    addAndMakeVisible(textureMenu);
    addAndMakeVisible(shapeASelector);
    addAndMakeVisible(shapeBSelector);

    setupOperationButtons();
    setupScanModeButtons();
    updateScanKnobLabels(static_cast<int>(processor.apvts.getRawParameterValue("scanMode")->load()));
    processor.apvts.addParameterListener("operation", this);
    processor.apvts.addParameterListener("scanMode", this);

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
    addAndMakeVisible(filterCutKnob);
    addAndMakeVisible(filterResKnob);
    addAndMakeVisible(attackKnob);
    addAndMakeVisible(decayKnob);
    addAndMakeVisible(sustainKnob);
    addAndMakeVisible(releaseKnob);
    addAndMakeVisible(gainKnob);
    addAndMakeVisible(adsrDisplay);
    addAndMakeVisible(waveformScope);

    setupSkyboxSelector();
    setupFilterModeSelector();

    // Tooltips for all knobs
    sizeAKnob.setTooltipText("Size of shape A (0.1 - 0.7)");
    sizeBKnob.setTooltipText("Size of shape B (0.1 - 0.7)");
    offsetXKnob.setTooltipText("Horizontal offset between shapes");
    offsetYKnob.setTooltipText("Vertical offset between shapes");
    smoothKKnob.setTooltipText("Smooth blend factor for operations");
    twistKnob.setTooltipText("Twist deformation amount");
    scanRadiusKnob.setTooltipText("Scan circle radius / ray range");
    scanHeightKnob.setTooltipText("Y-axis scan height position");
    topoMorphKnob.setTooltipText("Mode-specific: MRI blend / rays / bounces / density / bands / shape");
    distScaleKnob.setTooltipText("Distance-to-amplitude scaling factor");
    filterCutKnob.setTooltipText("Filter cutoff frequency (Hz)");
    filterResKnob.setTooltipText("Filter resonance amount");
    attackKnob.setTooltipText("ADSR attack time");
    decayKnob.setTooltipText("ADSR decay time");
    sustainKnob.setTooltipText("ADSR sustain level");
    releaseKnob.setTooltipText("ADSR release time");
    gainKnob.setTooltipText("Master output volume");
    skyExpKnob.setTooltipText("Skybox exposure brightness");
    skyRotKnob.setTooltipText("Skybox rotation angle");
    skyReflKnob.setTooltipText("Surface reflection intensity");
    skyBlurKnob.setTooltipText("Skybox blur amount");

    // Collapsible MIDI keyboard — dark teal theme, C0-C7 range
    midiKeyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xFF0E1A1A));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xFF060E0E));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, SDFLookAndFeel::borderColour.withAlpha(0.6f));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, SDFLookAndFeel::tertiaryAccent.withAlpha(0.3f));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, SDFLookAndFeel::tertiaryAccent.withAlpha(0.6f));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, SDFLookAndFeel::mutedText);
    midiKeyboard.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colour(0x40000000));
    midiKeyboard.setAvailableRange(12, 96); // C0 to C7
    midiKeyboard.setLowestVisibleKey(48); // C3 centered on startup
    addChildComponent(midiKeyboard); // starts hidden

    keyboardToggle.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
    keyboardToggle.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
    keyboardToggle.onClick = [this]()
    {
        keyboardVisible = !keyboardVisible;
        midiKeyboard.setVisible(keyboardVisible);
        keyboardToggle.setColour(juce::TextButton::textColourOffId,
            keyboardVisible ? SDFLookAndFeel::primaryAccent : SDFLookAndFeel::mutedText);
        resized();
    };
    addAndMakeVisible(keyboardToggle);

    setupModRouting();

    // IMPORTANT: setSize must be LAST — it triggers resized() which needs all children ready
    setResizable(true, true);
    setResizeLimits(900, 700, 1600, 1200);
    setSize(1000, 780);
    startTimerHz(30);
}

SDFSynthEditor::~SDFSynthEditor()
{
    processor.setTextureSystem(nullptr);
    processor.apvts.removeParameterListener("operation", this);
    processor.apvts.removeParameterListener("scanMode", this);
    stopTimer();
    setLookAndFeel(nullptr);
}

void SDFSynthEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "operation")
    {
        int idx = static_cast<int>(newValue);
        juce::MessageManager::callAsync([this, idx]()
        {
            if (idx >= 0 && idx < opButtons.size())
                opButtons[idx]->setToggleState(true, juce::dontSendNotification);
        });
    }
    else if (parameterID == "scanMode")
    {
        int idx = static_cast<int>(newValue);
        juce::MessageManager::callAsync([this, idx]()
        {
            if (idx >= 0 && idx < scanModeButtons.size())
                scanModeButtons[idx]->setToggleState(true, juce::dontSendNotification);
            updateScanKnobLabels(idx);
        });
    }
}

void SDFSynthEditor::setupOperationButtons()
{
    juce::StringArray opNames = { "SMOOTH", "UNION", "INTER", "SUB" };
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
            if (auto* param = processor.apvts.getParameter("operation"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
        };
        addAndMakeVisible(btn);
    }

    int initOp = static_cast<int>(processor.apvts.getRawParameterValue("operation")->load());
    if (initOp >= 0 && initOp < opButtons.size())
        opButtons[initOp]->setToggleState(true, juce::dontSendNotification);
}

void SDFSynthEditor::setupScanModeButtons()
{
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
            if (auto* param = processor.apvts.getParameter("scanMode"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
        };
        addAndMakeVisible(btn);
    }

    int initMode = static_cast<int>(processor.apvts.getRawParameterValue("scanMode")->load());
    if (initMode >= 0 && initMode < scanModeButtons.size())
        scanModeButtons[initMode]->setToggleState(true, juce::dontSendNotification);
}

void SDFSynthEditor::setupSkyboxSelector()
{
    auto names = SkyboxSystem::getPresetNames();
    skyboxSelector.addItem("None", 1);
    for (int i = 0; i < names.size(); ++i)
        skyboxSelector.addItem(names[i], i + 2);

    skyboxSelector.setSelectedId(1, juce::dontSendNotification);
    skyboxSelector.onChange = [this]()
    {
        int id = skyboxSelector.getSelectedId();
        if (id <= 1)
            viewport3D.skyboxSystem.clear();
        else
            viewport3D.skyboxSystem.loadPreset(id - 2);
    };
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
                    if (viewport3D.skyboxSystem.loadHDR(file))
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

void SDFSynthEditor::setupFilterModeSelector()
{
    juce::StringArray modeNames = { "Low Pass", "Band Pass", "High Pass", "Notch", "Peak" };
    for (int i = 0; i < modeNames.size(); ++i)
        filterModeSelector.addItem(modeNames[i], i + 1);

    int initMode = static_cast<int>(processor.apvts.getRawParameterValue("filterMode")->load());
    filterModeSelector.setSelectedId(initMode + 1, juce::dontSendNotification);

    filterModeSelector.onChange = [this]()
    {
        int id = filterModeSelector.getSelectedId();
        if (id > 0)
        {
            if (auto* param = processor.apvts.getParameter("filterMode"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(id - 1)));
        }
    };

    filterModeSelector.setColour(juce::ComboBox::backgroundColourId, SDFLookAndFeel::panelBg);
    filterModeSelector.setColour(juce::ComboBox::textColourId, SDFLookAndFeel::tertiaryAccent);
    filterModeSelector.setColour(juce::ComboBox::outlineColourId, SDFLookAndFeel::borderColour);
    addAndMakeVisible(filterModeSelector);
}

void SDFSynthEditor::applyPresetResources()
{
    auto texKey = presetManager.getLastTextureKey();
    if (texKey.isNotEmpty())
    {
        auto tex = ProceduralLibrary::generate(texKey);
        viewport3D.textureSystem.loadAllFromImages(
            tex.colorMap, tex.normalMap, tex.roughMap,
            tex.dispMap, tex.aoMap, tex.emitMap);
        processor.markWavetableDirty();
    }
    else
    {
        viewport3D.textureSystem.clearAll();
        processor.markWavetableDirty();
    }

    int skyIdx = presetManager.getLastSkyboxIndex();
    if (skyIdx >= 0 && skyIdx <= 5)
    {
        viewport3D.skyboxSystem.loadPreset(skyIdx);
        skyboxSelector.setSelectedId(skyIdx + 2, juce::dontSendNotification);
    }
    else
    {
        viewport3D.skyboxSystem.clear();
        skyboxSelector.setSelectedId(1, juce::dontSendNotification);
    }

    // Sync filter mode selector with preset
    int fmIdx = static_cast<int>(processor.apvts.getRawParameterValue("filterMode")->load());
    filterModeSelector.setSelectedId(fmIdx + 1, juce::dontSendNotification);
}

void SDFSynthEditor::updateScanKnobLabels(int mode)
{
    // Labels per mode: {scanRadius, scanHeight, topoMorph, distScale}
    switch (mode)
    {
        case 1: // RayMarchSonify
            scanRadiusKnob.setLabel("Range");
            scanHeightKnob.setLabel("Height");
            topoMorphKnob.setLabel("Rays");
            distScaleKnob.setLabel("Scale");
            break;
        case 2: // AcousticTrace
            scanRadiusKnob.setLabel("Mic");
            scanHeightKnob.setLabel("Height");
            topoMorphKnob.setLabel("Bounce");
            distScaleKnob.setLabel("Pulse");
            break;
        case 3: // GranularCurvature
            scanRadiusKnob.setLabel("Radius");
            scanHeightKnob.setLabel("Height");
            topoMorphKnob.setLabel("Dense");
            distScaleKnob.setLabel("Curve");
            break;
        case 4: // VolumetricSpectro
            scanRadiusKnob.setLabel("Radius");
            scanHeightKnob.setLabel("Center");
            topoMorphKnob.setLabel("Bands");
            distScaleKnob.setLabel("Sharp");
            break;
        case 5: // FieldTraverse
            scanRadiusKnob.setLabel("Path");
            scanHeightKnob.setLabel("Center");
            topoMorphKnob.setLabel("Shape");
            distScaleKnob.setLabel("Scale");
            break;
        default: // Contour
            scanRadiusKnob.setLabel("Radius");
            scanHeightKnob.setLabel("Height");
            topoMorphKnob.setLabel("MRI");
            distScaleKnob.setLabel("Scale");
            break;
    }
}

void SDFSynthEditor::timerCallback()
{
    waveformScope.setWavetable(&processor.getCurrentWavetable());
    waveformScope.setContour(&processor.getCurrentContour());
    waveformScope.setPlayheadPhase(processor.getActivePhase());
    waveformScope.setScanHeight(processor.apvts.getRawParameterValue("scanHeight")->load());
    waveformScope.setTopoMorph(processor.apvts.getRawParameterValue("topoMorph")->load());
    waveformScope.setScanMode(static_cast<int>(processor.apvts.getRawParameterValue("scanMode")->load()));

    // Update voice activity meter
    int voiceCount = processor.getActiveVoiceCount();
    if (voiceCount != lastVoiceCount)
    {
        lastVoiceCount = voiceCount;
        repaint(voiceMeterBounds);
    }

    // Update live modulation visualization on knobs
    {
        float envValue = processor.getLastEnvelopeValue();
        for (auto* knob : modTargetKnobs)
        {
            float md = knob->getModDepth();
            if (md != 0.f)
                knob->setModLiveOffset(envValue * md);
            else
                knob->setModLiveOffset(0.f);
        }
    }

    // Repaint during mod drag for wire animation
    if (adsrDisplay.isModDragActive())
        repaint();
}

void SDFSynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(SDFLookAndFeel::bgColour);
    auto W = static_cast<float>(getWidth());

    // Header bar
    constexpr int hdrH = 32;
    g.setColour(SDFLookAndFeel::panelBg);
    g.fillRect(0, 0, getWidth(), hdrH);
    g.setColour(juce::Colour(0x20000000));
    g.drawLine(0.f, static_cast<float>(hdrH) + 1.f, W, static_cast<float>(hdrH) + 1.f);
    g.setColour(SDFLookAndFeel::borderColour);
    g.drawLine(0.f, static_cast<float>(hdrH), W, static_cast<float>(hdrH));

    g.setColour(SDFLookAndFeel::primaryAccent);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.f, juce::Font::bold));
    g.drawText("SDF SYNTH", 6, 0, 100, hdrH, juce::Justification::centredLeft);

    g.setColour(SDFLookAndFeel::mutedText);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.f, 0));
    g.drawText("3D TOPO SCAN", 108, 0, 80, hdrH, juce::Justification::centredLeft);

    // Voice activity meter in header
    {
        auto mb = voiceMeterBounds;
        if (!mb.isEmpty())
        {
            // Draw voice count LEDs
            int total = sdf::MAX_VOICES;
            int active = lastVoiceCount;
            int ledW = 4;
            int ledH = 6;
            int gap = 2;
            int startX = mb.getX();
            int cy = mb.getCentreY();

            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.f, 0));
            g.setColour(SDFLookAndFeel::mutedText);
            g.drawText("V:", startX - 14, mb.getY(), 14, mb.getHeight(), juce::Justification::centredRight);

            for (int i = 0; i < total; ++i)
            {
                int x = startX + i * (ledW + gap);
                if (i < active)
                    g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.9f));
                else
                    g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.4f));
                g.fillRect(x, cy - ledH / 2, ledW, ledH);
            }
        }
    }

    // Column divider: subtle vertical line at 60% width
    float divX = W * 0.60f;
    g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.5f));
    g.drawLine(divX, static_cast<float>(hdrH), divX, static_cast<float>(getHeight()));

    // Draw left-column VISUAL panel (texture | skybox)
    SDFLookAndFeel::drawPanel(g, visualPanelBounds, "VISUAL", SDFLookAndFeel::primaryAccent);

    // Vertical divider between texture and skybox halves
    {
        auto inner = visualPanelBounds.reduced(8, 4);
        inner.removeFromTop(14);
        int texW = static_cast<int>(inner.getWidth() * 0.60f);
        float splitX = static_cast<float>(inner.getX() + texW + 3);
        g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.3f));
        g.drawLine(splitX, static_cast<float>(inner.getY()),
                   splitX, static_cast<float>(inner.getBottom()));
    }

    // Draw right-column panels
    SDFLookAndFeel::drawPanel(g, shapePanelBounds, "SHAPE", SDFLookAndFeel::primaryAccent);
    SDFLookAndFeel::drawPanel(g, scenePanelBounds, "SCENE", SDFLookAndFeel::primaryAccent);
    SDFLookAndFeel::drawPanel(g, scanPanelBounds, "SCAN", SDFLookAndFeel::secondaryAccent);
    SDFLookAndFeel::drawPanel(g, filterEnvPanelBounds, "FILTER & ENVELOPE", SDFLookAndFeel::tertiaryAccent);

    // Mod drag wire
    if (adsrDisplay.isModDragActive())
    {
        auto handleCentre = adsrDisplay.getModHandleCentre();
        auto start = adsrDisplay.localPointToGlobal(handleCentre);
        start = getLocalPoint(nullptr, start);
        auto end = adsrDisplay.getModDragPos();

        juce::Path wire;
        wire.startNewSubPath(start.toFloat());
        float midY = (start.y + end.y) * 0.5f;
        wire.cubicTo(static_cast<float>(start.x), midY,
                     static_cast<float>(end.x), midY,
                     static_cast<float>(end.x), static_cast<float>(end.y));
        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.7f));
        g.strokePath(wire, juce::PathStrokeType(2.f));
    }
}

void SDFSynthEditor::resized()
{
    constexpr int hdrH = 32;
    constexpr int margin = 6;
    constexpr int pad = 8;

    auto W = getWidth();
    auto bounds = getLocalBounds();

    // === Header bar ===
    {
        auto hdr = bounds.removeFromTop(hdrH);
        hdr.removeFromLeft(190); // after title text

        // KB toggle button
        keyboardToggle.setBounds(hdr.removeFromLeft(32).reduced(2, 4));
        hdr.removeFromLeft(4);

        // Voice meter: 16 LEDs * (4+2) = 96px + 14px label = 110px
        auto voiceArea = hdr.removeFromLeft(124).reduced(4, 8);
        voiceMeterBounds = voiceArea;
        voiceMeterBounds.removeFromLeft(14); // space for "V:" label

        auto presetArea = hdr.reduced(4, 4);
        presetLoadBtn.setBounds(presetArea.removeFromRight(40));
        presetArea.removeFromRight(2);
        presetSaveBtn.setBounds(presetArea.removeFromRight(40));
        presetArea.removeFromRight(4);
        presetSelector.setBounds(presetArea);
    }

    auto contentArea = bounds;

    // Collapsible keyboard at bottom
    if (keyboardVisible)
        midiKeyboard.setBounds(contentArea.removeFromBottom(kKeyboardHeight));

    int contentH = contentArea.getHeight();

    // Two-column split
    int leftW = static_cast<int>(W * 0.60f);

    auto leftCol = contentArea.removeFromLeft(leftW);
    auto rightCol = contentArea;

    // ============================================================
    // LEFT COLUMN: viewport (50%), waveform (22%), visual (rest)
    // ============================================================
    {
        int vpH = static_cast<int>(contentH * 0.50f);
        int wfH = static_cast<int>(contentH * 0.22f);

        viewport3D.setBounds(leftCol.removeFromTop(vpH).reduced(margin, margin));
        waveformScope.setBounds(leftCol.removeFromTop(wfH).reduced(margin, margin / 2));

        // --- VISUAL PANEL (texture left | skybox right) ---
        visualPanelBounds = leftCol.reduced(margin, margin / 2);
        {
            auto inner = visualPanelBounds.reduced(pad, 4);
            inner.removeFromTop(14); // panel title

            // Split: texture 60%, skybox 40%
            int texW = static_cast<int>(inner.getWidth() * 0.60f);
            auto texSide = inner.removeFromLeft(texW);
            inner.removeFromLeft(pad); // gap between sides
            auto skySide = inner;

            // Left: TextureMenu fills its column
            textureMenu.setBounds(texSide);

            // Right: skybox selector + HDR on top, 2x2 knobs below
            auto skySelRow = skySide.removeFromTop(20);
            skyLoadBtn.setBounds(skySelRow.removeFromRight(32));
            skySelRow.removeFromRight(4);
            skyboxSelector.setBounds(skySelRow);

            skySide.removeFromTop(4);

            // 2x2 knob grid
            int skyKW = skySide.getWidth() / 2;
            int skyKH = skySide.getHeight() / 2;

            auto skyR1 = skySide.removeFromTop(skyKH);
            skyExpKnob.setBounds(skyR1.removeFromLeft(skyKW));
            skyRotKnob.setBounds(skyR1);

            auto skyR2 = skySide;
            skyReflKnob.setBounds(skyR2.removeFromLeft(skyKW));
            skyBlurKnob.setBounds(skyR2);
        }
    }

    // ============================================================
    // RIGHT COLUMN: 4 panels stacked vertically
    // ============================================================
    auto rc = rightCol.reduced(margin, margin);
    int rcH = rc.getHeight();

    // Panel height proportions: Shape 13%, Scene 23%, Scan 27%, FilterEnv rest (~37%)
    int shapeH   = static_cast<int>(rcH * 0.13f);
    int sceneH   = static_cast<int>(rcH * 0.23f);
    int scanH    = static_cast<int>(rcH * 0.27f);

    // --- SHAPE PANEL ---
    shapePanelBounds = rc.removeFromTop(shapeH);
    rc.removeFromTop(margin);
    {
        auto inner = shapePanelBounds.reduced(pad, 0);
        inner.removeFromTop(18); // title space

        auto saRow = inner.removeFromTop(20);
        shapeASelector.setBounds(saRow);

        auto opRow = inner.removeFromTop(20);
        if (opButtons.size() > 0)
        {
            int opBtnW = opRow.getWidth() / opButtons.size();
            for (auto* btn : opButtons)
                btn->setBounds(opRow.removeFromLeft(opBtnW));
        }

        shapeBSelector.setBounds(inner.removeFromTop(20));
    }

    // --- SCENE PANEL ---
    scenePanelBounds = rc.removeFromTop(sceneH);
    rc.removeFromTop(margin);
    {
        auto inner = scenePanelBounds.reduced(pad, 0);
        inner.removeFromTop(18); // title

        // 3x2 grid of knobs
        int knobW = inner.getWidth() / 3;
        int knobH = inner.getHeight() / 2;

        auto row1 = inner.removeFromTop(knobH);
        sizeAKnob.setBounds(row1.removeFromLeft(knobW));
        sizeBKnob.setBounds(row1.removeFromLeft(knobW));
        offsetXKnob.setBounds(row1);

        auto row2 = inner;
        offsetYKnob.setBounds(row2.removeFromLeft(knobW));
        smoothKKnob.setBounds(row2.removeFromLeft(knobW));
        twistKnob.setBounds(row2);
    }

    // --- SCAN PANEL ---
    scanPanelBounds = rc.removeFromTop(scanH);
    rc.removeFromTop(margin);
    {
        auto inner = scanPanelBounds.reduced(pad, 0);
        inner.removeFromTop(18); // title

        // Scan mode buttons: 3x2 grid
        int scanBtnRows = 2;
        int scanBtnCols = 3;
        int sbtnH = 20;
        for (int r = 0; r < scanBtnRows; ++r)
        {
            auto btnRow = inner.removeFromTop(sbtnH);
            for (int c = 0; c < scanBtnCols; ++c)
            {
                int idx = r * scanBtnCols + c;
                if (idx < scanModeButtons.size())
                {
                    int btnW = btnRow.getWidth() / (scanBtnCols - c);
                    scanModeButtons[idx]->setBounds(btnRow.removeFromLeft(btnW));
                }
            }
        }

        inner.removeFromTop(4);

        // Scan knobs: 2x2 grid
        int knobW = inner.getWidth() / 2;
        int knobH = inner.getHeight() / 2;

        auto kr1 = inner.removeFromTop(knobH);
        scanRadiusKnob.setBounds(kr1.removeFromLeft(knobW));
        scanHeightKnob.setBounds(kr1);

        auto kr2 = inner;
        topoMorphKnob.setBounds(kr2.removeFromLeft(knobW));
        distScaleKnob.setBounds(kr2);
    }

    // --- FILTER & ENVELOPE PANEL ---
    filterEnvPanelBounds = rc;
    {
        auto inner = filterEnvPanelBounds.reduced(pad, 0);
        inner.removeFromTop(18); // title

        // Row 0: Filter mode selector (narrow row)
        auto fmRow = inner.removeFromTop(20);
        filterModeSelector.setBounds(fmRow);
        inner.removeFromTop(3);

        // Row 1: Filter cut + res + volume (3 knobs)
        int knobW3 = inner.getWidth() / 3;
        int knobRowH = juce::jmin(static_cast<int>(inner.getHeight() * 0.30f), 65);
        auto knobRow = inner.removeFromTop(knobRowH);
        filterCutKnob.setBounds(knobRow.removeFromLeft(knobW3));
        filterResKnob.setBounds(knobRow.removeFromLeft(knobW3));
        gainKnob.setBounds(knobRow);

        inner.removeFromTop(2);

        // Row 2: ADSR knobs (4 across) + graphic display side-by-side
        // Left half: 4 ADSR knobs in 2x2 grid
        // Right half: graphic ADSR display
        int adsrKnobW = inner.getWidth() / 2;
        auto adsrKnobArea = inner.removeFromLeft(adsrKnobW);
        auto adsrDisplayArea = inner;

        // 2x2 grid for ADSR knobs
        int akW = adsrKnobArea.getWidth() / 2;
        int akH = adsrKnobArea.getHeight() / 2;
        auto akR1 = adsrKnobArea.removeFromTop(akH);
        attackKnob.setBounds(akR1.removeFromLeft(akW));
        decayKnob.setBounds(akR1);
        auto akR2 = adsrKnobArea;
        sustainKnob.setBounds(akR2.removeFromLeft(akW));
        releaseKnob.setBounds(akR2);

        // Graphic ADSR display
        adsrDisplay.setBounds(adsrDisplayArea);
    }
}

void SDFSynthEditor::setupModRouting()
{
    // All non-visual knobs are mod targets: scene + scan + filter/env
    modTargetKnobs = {
        // Scene
        &sizeAKnob, &sizeBKnob, &offsetXKnob, &offsetYKnob, &smoothKKnob, &twistKnob,
        // Scan
        &scanRadiusKnob, &scanHeightKnob, &topoMorphKnob, &distScaleKnob,
        // Filter & Envelope
        &filterCutKnob, &filterResKnob,
        &attackKnob, &decayKnob, &sustainKnob, &releaseKnob,
        &gainKnob
    };

    for (auto* knob : modTargetKnobs)
    {
        knob->setModTargetEnabled(true);

        knob->onModRouteRemoved = [this, knob]()
        {
            processor.removeModRoute(knob->getParameterID());
            updateModDepthDisplays();
        };

        knob->onModDepthChanged = [this, knob](float newDepth)
        {
            processor.setModDepth(knob->getParameterID(), newDepth);
            updateModDepthDisplays();
        };
    }

    // ADSR Display mod drag callbacks
    adsrDisplay.onModDragStarted = [this]()
    {
        currentModHighlight = nullptr;
    };

    adsrDisplay.onModDragging = [this](juce::Point<int> pos)
    {
        auto* knob = findKnobAt(pos);
        if (knob != currentModHighlight)
        {
            if (currentModHighlight)
                currentModHighlight->setModDragHighlight(false);
            currentModHighlight = knob;
            if (currentModHighlight)
                currentModHighlight->setModDragHighlight(true);
        }
    };

    adsrDisplay.onModDragEnded = [this](juce::Point<int> pos)
    {
        if (currentModHighlight)
            currentModHighlight->setModDragHighlight(false);

        auto* knob = findKnobAt(pos);
        if (knob)
        {
            // Toggle: if already modulated, remove; otherwise add
            if (knob->getModDepth() != 0.f)
                processor.removeModRoute(knob->getParameterID());
            else
                processor.addModRoute(knob->getParameterID(), 0.5f);
            updateModDepthDisplays();
        }
        currentModHighlight = nullptr;
    };

    // Initialize mod depth displays from existing routes
    updateModDepthDisplays();
}

void SDFSynthEditor::updateModDepthDisplays()
{
    auto routes = processor.getModRoutes();

    for (auto* knob : modTargetKnobs)
    {
        float depth = 0.f;
        for (const auto& r : routes)
        {
            if (r.targetParamId == knob->getParameterID())
            {
                depth = r.depth;
                break;
            }
        }
        knob->setModDepth(depth);
    }
}

ArcKnob* SDFSynthEditor::findKnobAt(juce::Point<int> pos)
{
    for (auto* knob : modTargetKnobs)
    {
        auto knobBounds = knob->getBoundsInParent();
        if (knobBounds.contains(pos))
            return knob;
    }
    return nullptr;
}
