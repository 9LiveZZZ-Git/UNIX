#include "TextureMenu.h"

TextureMenu::TextureMenu(juce::AudioProcessorValueTreeState& a,
                         TextureSystem& ts)
    : apvts(a), textureSystem(ts),
      texScaleKnob(a, "texScale", "TxS", SDFLookAndFeel::primaryAccent),
      texBrightKnob(a, "texBright", "Brt", SDFLookAndFeel::primaryAccent),
      texBlendKnob(a, "texBlend", "Bld", SDFLookAndFeel::primaryAccent),
      normIntKnob(a, "normIntensity", "Nrm", SDFLookAndFeel::primaryAccent),
      roughOffKnob(a, "roughOffset", "Rgh", SDFLookAndFeel::primaryAccent),
      dispAmtKnob(a, "dispAmt", "Dsp", SDFLookAndFeel::secondaryAccent),
      aoIntKnob(a, "aoIntensity", "AO", SDFLookAndFeel::primaryAccent),
      emIntKnob(a, "emIntensity", "Emt", SDFLookAndFeel::secondaryAccent)
{
    // Preset selector
    presetSelector.addItem("None", 1);
    auto names = ProceduralLibrary::getPresetNames();
    for (int i = 0; i < names.size(); ++i)
        presetSelector.addItem(names[i], i + 2);

    // Scan for external PBR presets (pine forest pack)
    auto pineDir = juce::File("C:\\Users\\lpfre\\Downloads\\pine_forest\\textures");
    if (pineDir.isDirectory())
        scanExternalPresets(pineDir);

    // Add external presets to selector after a separator
    if (!externalPresets.empty())
    {
        auto proceduralCount = names.size();
        presetSelector.addSeparator();
        externalIdOffset = static_cast<int>(proceduralCount) + 3; // None(1) + procedurals(2..N+1) + separator
        for (int i = 0; i < static_cast<int>(externalPresets.size()); ++i)
            presetSelector.addItem(externalPresets[i].displayName, externalIdOffset + i);
    }

    presetSelector.setSelectedId(1, juce::dontSendNotification);
    presetSelector.onChange = [this]()
    {
        int id = presetSelector.getSelectedId();
        if (id <= 1)
        {
            textureSystem.clearAll();
            if (onTextureChanged) onTextureChanged();
        }
        else if (externalIdOffset > 0 && id >= externalIdOffset)
        {
            loadExternalPreset(id - externalIdOffset);
        }
        else
        {
            loadPreset(id - 2);
        }
    };
    addAndMakeVisible(presetSelector);

    // Upload buttons
    auto setupBtn = [this](juce::TextButton& btn, TextureSlot& slot)
    {
        btn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
        btn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::mutedText);
        btn.onClick = [this, &slot]() { browseForTexture(slot); };
        addAndMakeVisible(btn);
    };

    setupBtn(colorBtn, textureSystem.colorTex);
    setupBtn(normalBtn, textureSystem.normalTex);
    setupBtn(roughBtn, textureSystem.roughTex);
    setupBtn(dispBtn, textureSystem.dispTex);
    setupBtn(aoBtn, textureSystem.aoTex);
    setupBtn(emitBtn, textureSystem.emitTex);

    clearBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x40, 0x20, 0x20));
    clearBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::secondaryAccent);
    clearBtn.onClick = [this]()
    {
        textureSystem.clearAll();
        presetSelector.setSelectedId(1, juce::dontSendNotification);
        if (onTextureChanged) onTextureChanged();
    };
    addAndMakeVisible(clearBtn);

    // Material folder loader (auto-detects PBR channels from naming convention)
    matBtn.setColour(juce::TextButton::buttonColourId, SDFLookAndFeel::panelBg);
    matBtn.setColour(juce::TextButton::textColourOffId, SDFLookAndFeel::tertiaryAccent);
    matBtn.onClick = [this]() { browseForMaterial(); };
    addAndMakeVisible(matBtn);

    // Knobs
    addAndMakeVisible(texScaleKnob);
    addAndMakeVisible(texBrightKnob);
    addAndMakeVisible(texBlendKnob);
    addAndMakeVisible(normIntKnob);
    addAndMakeVisible(roughOffKnob);
    addAndMakeVisible(dispAmtKnob);
    addAndMakeVisible(aoIntKnob);
    addAndMakeVisible(emIntKnob);
}

void TextureMenu::loadPreset(int presetIndex)
{
    auto keys = ProceduralLibrary::getPresetKeys();
    if (presetIndex < 0 || presetIndex >= keys.size())
        return;

    auto tex = ProceduralLibrary::generate(keys[presetIndex]);

    textureSystem.loadAllFromImages(
        tex.colorMap, tex.normalMap, tex.roughMap,
        tex.dispMap, tex.aoMap, tex.emitMap);

    if (onTextureChanged)
        onTextureChanged();
}

void TextureMenu::browseForTexture(TextureSlot& slot)
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select texture image", juce::File{}, "*.png;*.jpg;*.jpeg;*.bmp");

    chooser->launchAsync(juce::FileBrowserComponent::openMode
                       | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser, &slot](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                auto img = juce::ImageFileFormat::loadFrom(file);
                if (img.isValid())
                {
                    textureSystem.loadFromImage(slot, img);
                    if (onTextureChanged)
                        onTextureChanged();
                }
            }
        });
}

void TextureMenu::browseForMaterial()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select any texture from the material (e.g. *_diff.png)",
        juce::File{}, "*.png;*.jpg;*.jpeg;*.bmp");

    chooser->launchAsync(juce::FileBrowserComponent::openMode
                       | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (!file.existsAsFile()) return;

            auto dir = file.getParentDirectory();
            auto name = file.getFileNameWithoutExtension();

            // Strip known suffixes to get the base material name
            juce::StringArray suffixes = {
                "_diff", "_diffuse", "_color", "_col", "_albedo",
                "_nor_gl", "_nor", "_normal", "_nrm",
                "_rough", "_roughness", "_rgh",
                "_disp", "_displacement", "_height",
                "_ao", "_ambient_occlusion",
                "_emit", "_emissive", "_glow",
                "_alpha", "_mask", "_mask01"
            };

            juce::String baseName = name;
            for (auto& sfx : suffixes)
            {
                if (name.endsWithIgnoreCase(sfx))
                {
                    baseName = name.substring(0, name.length() - sfx.length());
                    break;
                }
            }

            auto findMap = [&](const juce::StringArray& tags) -> juce::File
            {
                for (auto& tag : tags)
                {
                    for (auto& ext : { ".png", ".jpg", ".jpeg", ".bmp" })
                    {
                        auto candidate = dir.getChildFile(baseName + tag + ext);
                        if (candidate.existsAsFile())
                            return candidate;
                    }
                }
                return {};
            };

            auto loadChannel = [&](TextureSlot& slot, const juce::StringArray& tags)
            {
                auto f = findMap(tags);
                if (f.existsAsFile())
                {
                    auto img = juce::ImageFileFormat::loadFrom(f);
                    if (img.isValid())
                        textureSystem.loadFromImage(slot, img);
                }
            };

            // Clear existing first
            textureSystem.clearAll();

            // Load each channel by naming convention
            loadChannel(textureSystem.colorTex,  { "_diff", "_diffuse", "_color", "_col", "_albedo" });
            loadChannel(textureSystem.normalTex, { "_nor_gl", "_nor", "_normal", "_nrm", "_diffuse_normal" });
            loadChannel(textureSystem.roughTex,  { "_rough", "_roughness", "_rgh" });
            loadChannel(textureSystem.dispTex,   { "_disp", "_displacement", "_height" });
            loadChannel(textureSystem.aoTex,     { "_ao", "_ambient_occlusion" });
            loadChannel(textureSystem.emitTex,   { "_emit", "_emissive", "_glow" });

            presetSelector.setSelectedId(1, juce::dontSendNotification);
            if (onTextureChanged)
                onTextureChanged();
        });
}

void TextureMenu::paint(juce::Graphics&)
{
    // Panel background drawn by parent editor (VISUAL panel)
}

void TextureMenu::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    // Row 1: Preset selector (compact) + MAT + clear
    auto row1 = bounds.removeFromTop(20);
    clearBtn.setBounds(row1.removeFromRight(32));
    row1.removeFromRight(2);
    matBtn.setBounds(row1.removeFromRight(32));
    row1.removeFromRight(4);
    presetSelector.setBounds(row1.removeFromLeft(juce::jmin(160, row1.getWidth())));

    bounds.removeFromTop(4);

    // Row 2: Upload buttons
    auto row2 = bounds.removeFromTop(18);
    int btnW = row2.getWidth() / 6;
    colorBtn.setBounds(row2.removeFromLeft(btnW));
    normalBtn.setBounds(row2.removeFromLeft(btnW));
    roughBtn.setBounds(row2.removeFromLeft(btnW));
    dispBtn.setBounds(row2.removeFromLeft(btnW));
    aoBtn.setBounds(row2.removeFromLeft(btnW));
    emitBtn.setBounds(row2);

    bounds.removeFromTop(4);

    // Rows 3-4: 8 texture knobs in 2x4 grid
    auto knobGrid = bounds;
    int knobW = knobGrid.getWidth() / 4;
    int knobH = knobGrid.getHeight() / 2;

    auto topRow = knobGrid.removeFromTop(knobH);
    texScaleKnob.setBounds(topRow.removeFromLeft(knobW));
    texBrightKnob.setBounds(topRow.removeFromLeft(knobW));
    texBlendKnob.setBounds(topRow.removeFromLeft(knobW));
    normIntKnob.setBounds(topRow.removeFromLeft(knobW));

    auto botRow = knobGrid;
    roughOffKnob.setBounds(botRow.removeFromLeft(knobW));
    dispAmtKnob.setBounds(botRow.removeFromLeft(knobW));
    aoIntKnob.setBounds(botRow.removeFromLeft(knobW));
    emIntKnob.setBounds(botRow.removeFromLeft(knobW));
}

void TextureMenu::scanExternalPresets(const juce::File& dir)
{
    juce::StringArray suffixes = {
        "_diff", "_diffuse", "_color", "_col", "_albedo",
        "_nor_gl", "_nor", "_normal", "_nrm", "_diffuse_normal",
        "_rough", "_roughness", "_rgh",
        "_disp", "_displacement", "_height",
        "_ao", "_ambient_occlusion",
        "_emit", "_emissive", "_glow",
        "_alpha", "_mask", "_mask01",
        "_tiled_diff", "_tiled_disp", "_tiled_nor_gl", "_tiled_rough",
        "_dry_diff"
    };

    juce::StringArray foundBases;

    for (auto& file : dir.findChildFiles(juce::File::findFiles, false, "*.png;*.jpg;*.jpeg;*.bmp"))
    {
        auto name = file.getFileNameWithoutExtension();
        juce::String baseName = name;
        for (auto& sfx : suffixes)
        {
            if (name.endsWithIgnoreCase(sfx))
            {
                baseName = name.substring(0, name.length() - sfx.length());
                break;
            }
        }

        if (!foundBases.contains(baseName))
            foundBases.add(baseName);
    }

    foundBases.sort(false);

    for (auto& base : foundBases)
    {
        // Build a readable display name: replace underscores, title-case
        auto display = base.replace("_", " ").trim();
        if (display.isNotEmpty())
            display = display.substring(0, 1).toUpperCase() + display.substring(1);

        externalPresets.push_back({ display, dir, base });
    }
}

void TextureMenu::loadExternalPreset(int externalIndex)
{
    if (externalIndex < 0 || externalIndex >= static_cast<int>(externalPresets.size()))
        return;

    auto& preset = externalPresets[externalIndex];
    auto dir = preset.directory;
    auto baseName = preset.baseName;

    auto findMap = [&](const juce::StringArray& tags) -> juce::File
    {
        for (auto& tag : tags)
        {
            for (auto& ext : { ".png", ".jpg", ".jpeg", ".bmp" })
            {
                auto candidate = dir.getChildFile(baseName + tag + ext);
                if (candidate.existsAsFile())
                    return candidate;
            }
        }
        return {};
    };

    auto loadChannel = [&](TextureSlot& slot, const juce::StringArray& tags)
    {
        auto f = findMap(tags);
        if (f.existsAsFile())
        {
            auto img = juce::ImageFileFormat::loadFrom(f);
            if (img.isValid())
                textureSystem.loadFromImage(slot, img);
        }
    };

    textureSystem.clearAll();

    loadChannel(textureSystem.colorTex,  { "_diff", "_diffuse", "_color", "_col", "_albedo" });
    loadChannel(textureSystem.normalTex, { "_nor_gl", "_nor", "_normal", "_nrm", "_diffuse_normal" });
    loadChannel(textureSystem.roughTex,  { "_rough", "_roughness", "_rgh" });
    loadChannel(textureSystem.dispTex,   { "_disp", "_displacement", "_height" });
    loadChannel(textureSystem.aoTex,     { "_ao", "_ambient_occlusion" });
    loadChannel(textureSystem.emitTex,   { "_emit", "_emissive", "_glow" });

    if (onTextureChanged)
        onTextureChanged();
}
