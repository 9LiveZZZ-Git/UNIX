#include "PluginEditor.h"

SDFSynthEditor::SDFSynthEditor(SDFSynthProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      presetManager(p.apvts),
      viewport3D(p.apvts),
      waveformScope(),
      shapeTab(p.apvts),
      oscTab(p.apvts),
      fxTab(p.apvts),
      modTab(p.apvts),
      visTab(p.apvts, viewport3D.textureSystem, viewport3D),
      bottomStrip(p.apvts),
      midiKeyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&lookAndFeel);

    // Wire texture system to processor for audio modulation
    processor.setTextureSystem(&viewport3D.textureSystem);
    visTab.getTextureMenu().onTextureChanged = [this]() { processor.markWavetableDirty(); };

    // Wire modulated param values to viewport for live visualization
    viewport3D.getModulatedValue = [this](const char* paramId) -> float {
        return processor.getModulatedParamValue(juce::String(paramId));
    };

    // Apply saved UI scale
    currentScale = processor.savedUiScale;
    SDFLookAndFeel::scaleFactor = currentScale;

    // === Header bar buttons ===
    presetBrowserBtn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
    presetBrowserBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::primaryAccent);
    presetBrowserBtn.onClick = [this]() { showPresetMenu(); };
    addAndMakeVisible(presetBrowserBtn);

    initBtn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
    initBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
    initBtn.onClick = [this]()
    {
        presetManager.loadInitPreset();
        viewport3D.textureSystem.clearAll();
        viewport3D.skyboxSystem.clear();
        visTab.getSkyboxSelector().setSelectedId(1, juce::dontSendNotification);
        processor.markWavetableDirty();
        presetBrowserBtn.setButtonText("-- Preset --");
    };
    addAndMakeVisible(initBtn);

    abBtn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
    abBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
    abBtn.onClick = [this]()
    {
        if (!presetManager.isOnB())
        {
            presetManager.snapshotA();
            presetManager.loadB();
            abBtn.setButtonText("B");
            abBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::secondaryAccent);
            applyPresetResources();
        }
        else
        {
            presetManager.snapshotB();
            presetManager.loadA();
            abBtn.setButtonText("A");
            abBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
            applyPresetResources();
        }
    };
    addAndMakeVisible(abBtn);

    // Scale selector
    scaleSelector.addItem("75%", 1);
    scaleSelector.addItem("100%", 2);
    scaleSelector.addItem("125%", 3);
    scaleSelector.addItem("150%", 4);
    int initScaleId = (currentScale < 0.9f) ? 1 : (currentScale < 1.1f) ? 2 : (currentScale < 1.35f) ? 3 : 4;
    scaleSelector.setSelectedId(initScaleId, juce::dontSendNotification);
    scaleSelector.setColour(juce::ComboBox::backgroundColourId, SDFLookAndFeel::panelBg);
    scaleSelector.setColour(juce::ComboBox::textColourId, SDFLookAndFeel::mutedText);
    scaleSelector.setColour(juce::ComboBox::outlineColourId, SDFLookAndFeel::borderColour);
    scaleSelector.onChange = [this]()
    {
        int id = scaleSelector.getSelectedId();
        float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f };
        if (id >= 1 && id <= 4)
            applyScale(scales[id - 1]);
    };
    addAndMakeVisible(scaleSelector);

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

    // OBJ import button
    objLoadBtn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
    objLoadBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
    objLoadBtn.onClick = [this]()
    {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Load OBJ mesh", juce::File{}, "*.obj");
        chooser->launchAsync(juce::FileBrowserComponent::openMode
                           | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                    processor.importOBJFile(file);
            });
    };
    addAndMakeVisible(objLoadBtn);

    // Debug bypass buttons
    bypassOscBtn.setClickingTogglesState(true);
    bypassOscBtn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
    bypassOscBtn.setColour(juce::TextButton::buttonOnColourId, SDFLookAndFeel::meterRed);
    bypassOscBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::meterRed.withAlpha(0.5f));
    bypassOscBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    bypassOscBtn.onClick = [this]()
    {
        processor.bypassOsc.store(bypassOscBtn.getToggleState(), std::memory_order_relaxed);
    };
    addAndMakeVisible(bypassOscBtn);

    bypassFXBtn.setClickingTogglesState(true);
    bypassFXBtn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
    bypassFXBtn.setColour(juce::TextButton::buttonOnColourId, SDFLookAndFeel::meterRed);
    bypassFXBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::meterRed.withAlpha(0.5f));
    bypassFXBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    bypassFXBtn.onClick = [this]()
    {
        processor.bypassFX.store(bypassFXBtn.getToggleState(), std::memory_order_relaxed);
    };
    addAndMakeVisible(bypassFXBtn);

    // MIDI keyboard
    midiKeyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xFF0E1A1A));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xFF060E0E));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, SDFLookAndFeel::borderColour.withAlpha(0.6f));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, SDFLookAndFeel::tertiaryAccent.withAlpha(0.3f));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, SDFLookAndFeel::tertiaryAccent.withAlpha(0.6f));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, SDFLookAndFeel::mutedText);
    midiKeyboard.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colour(0x40000000));
    midiKeyboard.setAvailableRange(12, 96);
    midiKeyboard.setLowestVisibleKey(48);
    addChildComponent(midiKeyboard);

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

    // Main components
    addAndMakeVisible(viewport3D);
    addAndMakeVisible(waveformScope);
    addAndMakeVisible(tabBar);
    addAndMakeVisible(shapeTab);
    addAndMakeVisible(oscTab);
    addAndMakeVisible(fxTab);
    addAndMakeVisible(modTab);
    addAndMakeVisible(visTab);
    addAndMakeVisible(bottomStrip);

    // Tab switching
    tabBar.onTabChanged = [this](int idx) { switchTab(idx); };
    switchTab(0); // Start on SHAPE tab

    // Listen for scanMode/operation param changes
    processor.apvts.addParameterListener("scanMode", this);

    setupModRouting();

    // IMPORTANT: setSize must be LAST
    setResizable(true, true);
    setResizeLimits(SDFLookAndFeel::scaledInt(900), SDFLookAndFeel::scaledInt(700),
                    SDFLookAndFeel::scaledInt(1800), SDFLookAndFeel::scaledInt(1400));
    setSize(processor.savedWindowWidth, processor.savedWindowHeight);
    startTimerHz(30);
}

SDFSynthEditor::~SDFSynthEditor()
{
    processor.savedWindowWidth = getWidth();
    processor.savedWindowHeight = getHeight();
    processor.setTextureSystem(nullptr);
    processor.apvts.removeParameterListener("scanMode", this);
    stopTimer();
    setLookAndFeel(nullptr);
}

void SDFSynthEditor::switchTab(int tabIndex)
{
    shapeTab.setVisible(tabIndex == 0);
    oscTab.setVisible(tabIndex == 1);
    fxTab.setVisible(tabIndex == 2);
    modTab.setVisible(tabIndex == 3);
    visTab.setVisible(tabIndex == 4);
}

bool SDFSynthEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
        if (f.endsWithIgnoreCase(".obj"))
            return true;
    return false;
}

void SDFSynthEditor::filesDropped(const juce::StringArray& files, int, int)
{
    for (const auto& f : files)
    {
        if (f.endsWithIgnoreCase(".obj"))
        {
            processor.importOBJFile(juce::File(f));
            break;
        }
    }
}

void SDFSynthEditor::parameterChanged(const juce::String& parameterID, float)
{
    if (parameterID == "scanMode")
    {
        // Could update scan knob labels in ShapeTab if needed
    }
}

void SDFSynthEditor::timerCallback()
{
    waveformScope.setWavetable(&processor.getCurrentWavetable());
    waveformScope.setContour(processor.getCurrentContourPtr());
    waveformScope.setPlayheadPhase(processor.getActivePhase());
    waveformScope.setScanHeight(processor.apvts.getRawParameterValue("scanHeight")->load());
    waveformScope.setTopoMorph(processor.apvts.getRawParameterValue("topoMorph")->load());
    waveformScope.setScanMode(static_cast<int>(processor.apvts.getRawParameterValue("scanMode")->load()));

    int voiceCount = processor.getActiveVoiceCount();
    if (voiceCount != lastVoiceCount)
    {
        lastVoiceCount = voiceCount;
        repaint(voiceMeterBounds);
    }

    // Update mod indicators from mod matrix
    {
        auto& mm = processor.getModMatrix();
        for (auto* knob : modTargetKnobs)
        {
            int destIdx = SDFSynthProcessor::getDestIndex(knob->getParameterID());
            if (destIdx < 0) continue;

            ModulationMatrix::RouteInfo info[4];
            int count = mm.getRoutesForDest(destIdx, info, 4);

            modIndicatorScratch.clear();
            for (int i = 0; i < count; ++i)
            {
                ArcKnob::ModIndicator ind;
                ind.source = info[i].source;
                ind.depth = info[i].depth;
                ind.liveValue = mm.getSourceValue(info[i].source) * info[i].depth;
                ind.colour = ArcKnob::getModSourceColour(info[i].source);
                modIndicatorScratch.push_back(ind);
            }
            knob->setModIndicators(modIndicatorScratch);
            knob->updateModLiveValues(mm);
        }
    }

    // OBJ button status + forward voxel to viewport
    {
        if (processor.isVoxelizing())
        {
            objLoadBtn.setButtonText("...");
            objLoadBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::secondaryAccent);
        }
        else
        {
            auto meshName = processor.getCustomMeshName();
            if (meshName.isNotEmpty())
            {
                auto display = meshName.substring(0, 6);
                objLoadBtn.setButtonText(display);
                objLoadBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::primaryAccent);
            }
            else
            {
                objLoadBtn.setButtonText("OBJ");
                objLoadBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
            }
        }

        auto voxel = processor.getCurrentVoxelSDF();
        if (voxel)
            viewport3D.setVoxelSDF(voxel);
    }

    if (bottomStrip.getAdsrDisplay().isModDragActive())
        repaint();
}

void SDFSynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(SDFLookAndFeel::bgColour);
    auto W = static_cast<float>(getWidth());
    int hdrH = SDFLookAndFeel::scaledInt(40);

    // Header gradient
    {
        juce::ColourGradient hdrGrad(SDFLookAndFeel::panelBg, 0.f, 0.f,
                                      SDFLookAndFeel::bgColour, 0.f, static_cast<float>(hdrH), false);
        g.setGradientFill(hdrGrad);
        g.fillRect(0, 0, getWidth(), hdrH);
    }

    // Shadow line below header
    g.setColour(juce::Colour(0x30000000));
    g.drawLine(0.f, static_cast<float>(hdrH), W, static_cast<float>(hdrH), 1.f);

    // Title
    {
        auto titleFont = juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(15.f), juce::Font::bold);
        titleFont.setExtraKerningFactor(0.1f);
        g.setFont(titleFont);
        g.setColour(SDFLookAndFeel::primaryAccent);
        g.drawText("SDF SYNTH", SDFLookAndFeel::scaledInt(6), 0, SDFLookAndFeel::scaledInt(110), hdrH, juce::Justification::centredLeft);
    }

    // Bypass glow
    {
        auto drawBypassGlow = [&](juce::TextButton& btn)
        {
            if (!btn.getToggleState()) return;
            auto bb = btn.getBounds().toFloat().expanded(1.f);
            g.setColour(SDFLookAndFeel::meterRed.withAlpha(0.25f));
            g.fillRoundedRectangle(bb.expanded(2.f), SDFLookAndFeel::innerCorner + 2.f);
        };
        drawBypassGlow(bypassOscBtn);
        drawBypassGlow(bypassFXBtn);
    }

    // Voice activity meter
    {
        auto mb = voiceMeterBounds;
        if (!mb.isEmpty())
        {
            int total = sdf::MAX_VOICES;
            int active = lastVoiceCount;
            int ledW = SDFLookAndFeel::scaledInt(4);
            int ledH = SDFLookAndFeel::scaledInt(6);
            int gap = SDFLookAndFeel::scaledInt(2);
            int startX = mb.getX();
            int cy = mb.getCentreY();

            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(8.f), 0));
            g.setColour(SDFLookAndFeel::mutedText);
            g.drawText("V:", startX - SDFLookAndFeel::scaledInt(14), mb.getY(), SDFLookAndFeel::scaledInt(14), mb.getHeight(), juce::Justification::centredRight);

            for (int i = 0; i < total; ++i)
            {
                int x = startX + i * (ledW + gap);
                if (i < active)
                {
                    if (i < 6)
                        g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.9f));
                    else if (i < 11)
                        g.setColour(SDFLookAndFeel::meterYellow.withAlpha(0.9f));
                    else
                        g.setColour(SDFLookAndFeel::meterRed.withAlpha(0.9f));
                }
                else
                {
                    g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.4f));
                }
                g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(cy - ledH / 2),
                                        static_cast<float>(ledW), static_cast<float>(ledH), 1.f);
            }
        }
    }

    // Column divider
    float divX = W * 0.55f;
    {
        float divTop = static_cast<float>(hdrH);
        float divBot = static_cast<float>(getHeight());
        auto divCol = SDFLookAndFeel::borderColour.withAlpha(0.3f);
        g.setColour(divCol);
        g.drawLine(divX, divTop, divX, divBot, 1.f);
    }

    // Viewport frame
    {
        auto vpBoundsF = viewport3D.getBounds().toFloat();
        g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.4f));
        g.drawRoundedRectangle(vpBoundsF, SDFLookAndFeel::cornerRadius, 1.f);
    }

    // Voxelizing overlay
    if (processor.isVoxelizing())
    {
        auto vpBounds = viewport3D.getBounds();
        g.setColour(juce::Colour(0x80000000));
        g.fillRect(vpBounds);
        g.setColour(SDFLookAndFeel::primaryAccent);
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(16.f), juce::Font::bold));
        g.drawText("VOXELIZING...", vpBounds, juce::Justification::centred);
    }

    // Mod drag wire from ADSR display
    if (bottomStrip.getAdsrDisplay().isModDragActive())
    {
        auto& ad = bottomStrip.getAdsrDisplay();
        auto handleCentre = ad.getModHandleCentre();
        auto start = ad.localPointToGlobal(handleCentre);
        start = getLocalPoint(nullptr, start);
        auto end = ad.getModDragPos();

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
    int hdrH = SDFLookAndFeel::scaledInt(40);
    int margin = SDFLookAndFeel::scaledInt(6);
    int bottomH = SDFLookAndFeel::scaledInt(130);

    auto bounds = getLocalBounds();

    // === Header bar ===
    {
        auto hdr = bounds.removeFromTop(hdrH);
        hdr.removeFromLeft(SDFLookAndFeel::scaledInt(120));

        bypassOscBtn.setBounds(hdr.removeFromLeft(SDFLookAndFeel::scaledInt(52)).reduced(2, 5));
        hdr.removeFromLeft(SDFLookAndFeel::scaledInt(3));
        bypassFXBtn.setBounds(hdr.removeFromLeft(SDFLookAndFeel::scaledInt(44)).reduced(2, 5));
        hdr.removeFromLeft(SDFLookAndFeel::scaledInt(6));

        keyboardToggle.setBounds(hdr.removeFromLeft(SDFLookAndFeel::scaledInt(32)).reduced(2, 4));
        hdr.removeFromLeft(SDFLookAndFeel::scaledInt(4));
        objLoadBtn.setBounds(hdr.removeFromLeft(SDFLookAndFeel::scaledInt(40)).reduced(2, 4));
        hdr.removeFromLeft(SDFLookAndFeel::scaledInt(4));

        auto voiceArea = hdr.removeFromLeft(SDFLookAndFeel::scaledInt(124)).reduced(4, 8);
        voiceMeterBounds = voiceArea;
        voiceMeterBounds.removeFromLeft(SDFLookAndFeel::scaledInt(14));

        scaleSelector.setBounds(hdr.removeFromLeft(SDFLookAndFeel::scaledInt(56)).reduced(2, 4));
        hdr.removeFromLeft(SDFLookAndFeel::scaledInt(4));

        auto presetArea = hdr.reduced(4, 4);
        presetLoadBtn.setBounds(presetArea.removeFromRight(SDFLookAndFeel::scaledInt(40)));
        presetArea.removeFromRight(2);
        presetSaveBtn.setBounds(presetArea.removeFromRight(SDFLookAndFeel::scaledInt(40)));
        presetArea.removeFromRight(2);
        abBtn.setBounds(presetArea.removeFromRight(SDFLookAndFeel::scaledInt(28)));
        presetArea.removeFromRight(2);
        initBtn.setBounds(presetArea.removeFromRight(SDFLookAndFeel::scaledInt(36)));
        presetArea.removeFromRight(SDFLookAndFeel::scaledInt(4));
        presetBrowserBtn.setBounds(presetArea);
    }

    auto contentArea = bounds;

    if (keyboardVisible)
        midiKeyboard.setBounds(contentArea.removeFromBottom(SDFLookAndFeel::scaledInt(72)));

    // Bottom strip (persistent)
    bottomStrip.setBounds(contentArea.removeFromBottom(bottomH).reduced(margin, margin / 2));

    // Two-column split: 55% left, 45% right
    int leftW = static_cast<int>(contentArea.getWidth() * 0.55f);
    auto leftCol = contentArea.removeFromLeft(leftW);
    auto rightCol = contentArea;

    // === LEFT COLUMN: viewport + waveform ===
    {
        int wfH = SDFLookAndFeel::scaledInt(80);
        waveformScope.setBounds(leftCol.removeFromBottom(wfH).reduced(margin, margin / 2));
        viewport3D.setBounds(leftCol.reduced(margin, margin));
    }

    // === RIGHT COLUMN: tab bar + tab content ===
    {
        auto rc = rightCol.reduced(margin, margin);
        int tabBarH = SDFLookAndFeel::scaledInt(28);
        tabBar.setBounds(rc.removeFromTop(tabBarH));
        rc.removeFromTop(margin / 2);

        // All tab panels share the same bounds
        shapeTab.setBounds(rc);
        oscTab.setBounds(rc);
        fxTab.setBounds(rc);
        modTab.setBounds(rc);
        visTab.setBounds(rc);
    }

    repaint();
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
        visTab.getSkyboxSelector().setSelectedId(skyIdx + 2, juce::dontSendNotification);
    }
    else
    {
        viewport3D.skyboxSystem.clear();
        visTab.getSkyboxSelector().setSelectedId(1, juce::dontSendNotification);
    }
}

void SDFSynthEditor::setupModRouting()
{
    modTargetKnobs = {
        // Shape tab knobs
        &shapeTab.getSizeAKnob(), &shapeTab.getSizeBKnob(),
        &shapeTab.getOffsetXKnob(), &shapeTab.getOffsetYKnob(),
        &shapeTab.getSmoothKKnob(), &shapeTab.getTwistKnob(),
        &shapeTab.getScanRadiusKnob(), &shapeTab.getScanHeightKnob(),
        &shapeTab.getTopoMorphKnob(), &shapeTab.getDistScaleKnob(),
        // Bottom strip knobs
        &bottomStrip.getFilterCutKnob(), &bottomStrip.getFilterResKnob(),
        &bottomStrip.getAttackKnob(), &bottomStrip.getDecayKnob(),
        &bottomStrip.getSustainKnob(), &bottomStrip.getReleaseKnob(),
        &bottomStrip.getGainKnob(),
        // Osc tab knobs
        &oscTab.getOscFoldKnob(), &oscTab.getOscPDKnob(),
        &oscTab.getOscPWKnob(), &oscTab.getOscSyncKnob(),
        &oscTab.getUniDetuneKnob(), &oscTab.getUniSpreadKnob(),
        &oscTab.getOscBLevelKnob(), &oscTab.getOscBFMKnob(),
        &oscTab.getNoiseLevelKnob(), &oscTab.getNoiseFilterKnob(),
        // FX tab knobs
        &fxTab.getDistDriveKnob(), &fxTab.getChorusRateKnob(),
        &fxTab.getDelayFbKnob(), &fxTab.getReverbSizeKnob()
    };

    for (auto* knob : modTargetKnobs)
    {
        knob->setModTargetEnabled(true);

        knob->onModRouteRemoved = [this, knob](ModSource src)
        {
            int destIdx = SDFSynthProcessor::getDestIndex(knob->getParameterID());
            if (destIdx >= 0)
            {
                auto& mm = processor.getModMatrix();
                mm.copyReadToWrite();
                mm.removeRoute(src, static_cast<uint8_t>(destIdx));
                mm.publish();
            }
        };

        knob->onModDepthChanged = [this, knob](ModSource src, float newDepth)
        {
            int destIdx = SDFSynthProcessor::getDestIndex(knob->getParameterID());
            if (destIdx >= 0)
            {
                auto& mm = processor.getModMatrix();
                mm.copyReadToWrite();
                for (int i = 0; i < MAX_MOD_SLOTS; ++i)
                {
                    auto s = mm.getWriteSlot(i);
                    if (s.active && s.source == src && s.destIndex == static_cast<uint8_t>(destIdx))
                    {
                        ModSlot updated = s;
                        updated.depth = newDepth;
                        mm.setSlot(i, updated);
                        break;
                    }
                }
                mm.publish();
            }
        };
    }

    // ADSR display drag → envelope route
    auto& adsrDisplay = bottomStrip.getAdsrDisplay();
    adsrDisplay.onModDragStarted = [this]()
    {
        currentModHighlight = nullptr;
        currentDragSource = ModSource::Envelope;
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
            int destIdx = SDFSynthProcessor::getDestIndex(knob->getParameterID());
            if (destIdx >= 0)
            {
                auto& mm = processor.getModMatrix();
                mm.copyReadToWrite();

                bool exists = false;
                for (int i = 0; i < MAX_MOD_SLOTS; ++i)
                {
                    auto s = mm.getWriteSlot(i);
                    if (s.active && s.source == currentDragSource && s.destIndex == static_cast<uint8_t>(destIdx))
                    {
                        mm.clearSlot(i);
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                    mm.addRoute(currentDragSource, static_cast<uint8_t>(destIdx), 0.5f);

                mm.publish();
            }
        }
        currentModHighlight = nullptr;
    };
}

void SDFSynthEditor::updateModDepthDisplays()
{
    // Updates are done in timerCallback via mod matrix
}

ArcKnob* SDFSynthEditor::findKnobAt(juce::Point<int> pos)
{
    for (auto* knob : modTargetKnobs)
    {
        // Convert knob bounds to editor coordinates
        auto knobBounds = knob->getBoundsInParent();
        if (knob->getParentComponent() != this)
        {
            auto* parent = knob->getParentComponent();
            if (parent)
            {
                auto topLeft = parent->localPointToGlobal(knobBounds.getTopLeft());
                topLeft = getLocalPoint(nullptr, topLeft);
                knobBounds = juce::Rectangle<int>(topLeft.x, topLeft.y, knobBounds.getWidth(), knobBounds.getHeight());
            }
        }
        if (knobBounds.contains(pos))
            return knob;
    }
    return nullptr;
}

void SDFSynthEditor::applyScale(float newScale)
{
    currentScale = newScale;
    SDFLookAndFeel::scaleFactor = newScale;
    processor.savedUiScale = newScale;
    setResizeLimits(SDFLookAndFeel::scaledInt(900), SDFLookAndFeel::scaledInt(700),
                    SDFLookAndFeel::scaledInt(1800), SDFLookAndFeel::scaledInt(1400));
    setSize(SDFLookAndFeel::scaledInt(950), SDFLookAndFeel::scaledInt(760));
}

void SDFSynthEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    auto presets = PresetManager::getFactoryPresets();
    auto categories = PresetManager::getAllCategories();

    for (auto cat : categories)
    {
        juce::PopupMenu subMenu;
        for (int i = 0; i < static_cast<int>(presets.size()); ++i)
        {
            if (presets[static_cast<size_t>(i)].category == cat)
                subMenu.addItem(i + 1, presets[static_cast<size_t>(i)].name);
        }
        if (subMenu.getNumItems() > 0)
            menu.addSubMenu(PresetManager::getCategoryName(cat), subMenu);
    }

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetBrowserBtn),
        [this, presets](int result)
        {
            if (result > 0)
            {
                int idx = result - 1;
                presetManager.loadFactoryPreset(idx);
                applyPresetResources();
                presetBrowserBtn.setButtonText(presets[static_cast<size_t>(idx)].name);
            }
        });
}
