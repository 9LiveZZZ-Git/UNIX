#include "PresetManager.h"

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& a) : apvts(a) {}

juce::File PresetManager::getPresetsFolder() const
{
    auto folder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                    .getChildFile("SDF Synth").getChildFile("Presets");
    folder.createDirectory();
    return folder;
}

void PresetManager::savePreset(const juce::File& file)
{
    // Save as JSON-style XML with texture/skybox keys
    auto state = apvts.copyState();
    state.setProperty("textureKey", lastTextureKey, nullptr);
    state.setProperty("skyboxIndex", lastSkyboxIndex, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml)
        xml->writeTo(file);
}

bool PresetManager::loadPreset(const juce::File& file)
{
    auto xml = juce::XmlDocument::parse(file);
    if (!xml) return false;

    auto tree = juce::ValueTree::fromXml(*xml);
    if (tree.isValid() && tree.getType() == apvts.state.getType())
    {
        // Extract texture/skybox keys before replacing state
        lastTextureKey = tree.getProperty("textureKey", "").toString();
        lastSkyboxIndex = static_cast<int>(tree.getProperty("skyboxIndex", -1));
        lastSkyboxKey = "";

        apvts.replaceState(tree);
        return true;
    }
    return false;
}

juce::StringArray PresetManager::getFactoryPresetNames()
{
    juce::StringArray names;
    for (auto& p : getFactoryPresets())
        names.add(p.name);
    return names;
}

void PresetManager::loadFactoryPreset(int index)
{
    auto presets = getFactoryPresets();
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return;

    auto& preset = presets[index];

    for (auto& [paramId, value] : preset.params)
    {
        if (auto* param = apvts.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }

    // Store texture/skybox references for the editor to apply
    lastTextureKey = preset.textureKey;
    lastSkyboxIndex = preset.skyboxPreset;
}

void PresetManager::loadInitPreset()
{
    for (auto* p : apvts.processor.getParameters())
        p->setValueNotifyingHost(p->getDefaultValue());

    lastTextureKey = "";
    lastSkyboxIndex = -1;
}

void PresetManager::snapshotA()
{
    snapshotStateA = apvts.copyState();
    snapshotTexA = lastTextureKey;
    snapshotSkyA = lastSkyboxIndex;
}

void PresetManager::snapshotB()
{
    snapshotStateB = apvts.copyState();
    snapshotTexB = lastTextureKey;
    snapshotSkyB = lastSkyboxIndex;
}

void PresetManager::loadA()
{
    if (snapshotStateA.isValid())
    {
        apvts.replaceState(snapshotStateA);
        lastTextureKey = snapshotTexA;
        lastSkyboxIndex = snapshotSkyA;
        currentIsB = false;
    }
}

void PresetManager::loadB()
{
    if (snapshotStateB.isValid())
    {
        apvts.replaceState(snapshotStateB);
        lastTextureKey = snapshotTexB;
        lastSkyboxIndex = snapshotSkyB;
        currentIsB = true;
    }
}

juce::String PresetManager::getCategoryName(PresetCategory cat)
{
    switch (cat)
    {
        case PresetCategory::Pad:  return "PAD";
        case PresetCategory::Lead: return "LEAD";
        case PresetCategory::Bass: return "BASS";
        case PresetCategory::Key:  return "KEY";
        case PresetCategory::FX:   return "FX";
        case PresetCategory::Perc: return "PERC";
        default:                   return "PAD";
    }
}

std::vector<PresetCategory> PresetManager::getAllCategories()
{
    return { PresetCategory::Pad, PresetCategory::Lead, PresetCategory::Bass,
             PresetCategory::Key, PresetCategory::FX, PresetCategory::Perc };
}

std::vector<PresetManager::FactoryPreset> PresetManager::getFactoryPresets()
{
    return {
        // =================================================================
        //  30 Factory Presets
        //  Every preset sets ALL params to prevent leakage.
        //  Presets 1-15: original, with FX params added (all disabled).
        //  Presets 16-30: new, showcasing FX engine + categories.
        // =================================================================

        // 1. Obsidian Monolith -- massive black volcanic glass pillar in the void.
        //    Deep, dark, ominous drone. Oct+Box intersection -> angular facets.
        //    Gentle wavefolding for edge, brown noise rumble, 3 unison for thickness.
        { "Obsidian Monolith", {
            {"shape1", 4}, {"shape2", 1}, {"operation", 2}, {"smoothK", 0.3f},
            {"size1", 0.55f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.45f}, {"scanHeight", 0.f},
            {"topoMorph", 0.6f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.8f}, {"decay", 0.4f}, {"sustain", 0.9f}, {"release", 3.0f},
            {"masterGain", 0.2f}, {"filterCutoff", 3000.f}, {"filterRes", 0.15f},
            {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 8.f},
            {"unisonSpread", 40.f}, {"unisonBlend", 0.7f},
            {"oscFold", 0.15f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.06f},
            {"noiseType", 2.f}, {"noiseFilterCutoff", 800.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "obsidian", 0, PresetCategory::Pad },

        // 2. Crystal Cavern -- crystalline formations deep underground, light
        //    refracting through ice-blue facets. Shimmering, delicate.
        //    Oct-Torus subtraction -> crystal geode cavity. 5 unison for shimmer.
        { "Crystal Cavern", {
            {"shape1", 4}, {"shape2", 2}, {"operation", 3}, {"smoothK", 0.2f},
            {"size1", 0.5f}, {"size2", 0.3f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.85f}, {"distScale", 6.f}, {"scanMode", 3.f},
            {"attack", 0.01f}, {"decay", 0.8f}, {"sustain", 0.3f}, {"release", 2.5f},
            {"masterGain", 0.2f}, {"filterCutoff", 16000.f}, {"filterRes", 0.05f},
            {"filterMode", 0.f},
            {"unisonVoices", 5.f}, {"unisonDetune", 12.f},
            {"unisonSpread", 90.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.25f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.04f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 12000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "crystal", 5, PresetCategory::Pad },

        // 3. Lava Flow -- molten rock flowing through volcanic tubes, glowing
        //    orange-red. Aggressive, distorted, alive.
        //    Torus+Cyl smooth union -> volcanic tube. Heavy fold, ring mod crackle.
        { "Lava Flow", {
            {"shape1", 2}, {"shape2", 3}, {"operation", 0}, {"smoothK", 0.8f},
            {"size1", 0.5f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 1.2f}, {"scanRadius", 0.55f}, {"scanHeight", 0.05f},
            {"topoMorph", 0.9f}, {"distScale", 4.f}, {"scanMode", 1.f},
            {"attack", 0.01f}, {"decay", 0.15f}, {"sustain", 0.85f}, {"release", 0.3f},
            {"masterGain", 0.18f}, {"filterCutoff", 8000.f}, {"filterRes", 0.3f},
            {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 5.f},
            {"unisonSpread", 60.f}, {"unisonBlend", 0.6f},
            {"oscFold", 0.7f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.3f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.1f},
            {"noiseType", 1.f}, {"noiseFilterCutoff", 6000.f},
            {"oversample", 1.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "lava", 2, PresetCategory::Bass },

        // 4. Alien Artifact -- impossible geometric object found on a distant planet,
        //    pulsing with unknown energy. Weird, otherworldly.
        //    Sphere+Oct smooth union with heavy twist. FM eerie partials, PD vowels.
        { "Alien Artifact", {
            {"shape1", 0}, {"shape2", 4}, {"operation", 0}, {"smoothK", 0.7f},
            {"size1", 0.48f}, {"size2", 0.38f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 2.5f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.5f}, {"scanMode", 4.f},
            {"attack", 0.3f}, {"decay", 0.5f}, {"sustain", 0.8f}, {"release", 1.5f},
            {"masterGain", 0.2f}, {"filterCutoff", 8000.f}, {"filterRes", 0.15f},
            {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 18.f},
            {"unisonSpread", 70.f}, {"unisonBlend", 0.4f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.4f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.4f}, {"oscBSemitone", 7.f},
            {"oscBFine", 3.f}, {"oscBMixMode", 2.f}, {"oscBFMDepth", 0.3f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "alien", 4, PresetCategory::Lead },

        // 5. Marble Cathedral -- grand marble cathedral with soaring arches.
        //    Organ-like, reverberant, majestic.
        //    Cyl+Sphere smooth union -> pillared archway. Osc B octave up for organ.
        { "Marble Cathedral", {
            {"shape1", 3}, {"shape2", 0}, {"operation", 0}, {"smoothK", 1.0f},
            {"size1", 0.5f}, {"size2", 0.45f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.2f}, {"scanRadius", 0.48f}, {"scanHeight", 0.f},
            {"topoMorph", 0.65f}, {"distScale", 3.5f}, {"scanMode", 4.f},
            {"attack", 0.15f}, {"decay", 0.1f}, {"sustain", 0.92f}, {"release", 1.8f},
            {"masterGain", 0.22f}, {"filterCutoff", 10000.f}, {"filterRes", 0.08f},
            {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 6.f},
            {"unisonSpread", 50.f}, {"unisonBlend", 0.7f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.42f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.35f}, {"oscBSemitone", 12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "marble", 5, PresetCategory::Key },

        // 6. Plasma Storm -- violent plasma bolts crackling through a nebula.
        //    Electric, chaotic, intense lead.
        //    Torus+Oct union with twist. Heavy fold + sync, 6 unison wall of sound.
        { "Plasma Storm", {
            {"shape1", 2}, {"shape2", 4}, {"operation", 1}, {"smoothK", 0.5f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.3f}, {"offsetY", 0.f},
            {"twist", 1.8f}, {"scanRadius", 0.55f}, {"scanHeight", 0.f},
            {"topoMorph", 0.95f}, {"distScale", 5.f}, {"scanMode", 1.f},
            {"attack", 0.002f}, {"decay", 0.05f}, {"sustain", 0.95f}, {"release", 0.15f},
            {"masterGain", 0.16f}, {"filterCutoff", 14000.f}, {"filterRes", 0.35f},
            {"filterMode", 0.f},
            {"unisonVoices", 6.f}, {"unisonDetune", 20.f},
            {"unisonSpread", 85.f}, {"unisonBlend", 0.55f},
            {"oscFold", 0.55f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.4f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.05f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 15000.f},
            {"oversample", 1.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "plasma", 4, PresetCategory::Lead },

        // 7. Frozen Tundra -- vast icy plains stretching to the horizon under pale
        //    arctic light. Cold, ethereal, desolate pad.
        //    Sphere+Box smooth union -> rounded ice formation. 7 unison frozen shimmer.
        { "Frozen Tundra", {
            {"shape1", 0}, {"shape2", 1}, {"operation", 0}, {"smoothK", 0.9f},
            {"size1", 0.5f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.8f}, {"distScale", 2.5f}, {"scanMode", 0.f},
            {"attack", 1.5f}, {"decay", 0.6f}, {"sustain", 0.85f}, {"release", 4.0f},
            {"masterGain", 0.18f}, {"filterCutoff", 6000.f}, {"filterRes", 0.1f},
            {"filterMode", 0.f},
            {"unisonVoices", 7.f}, {"unisonDetune", 10.f},
            {"unisonSpread", 95.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.15f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.2f}, {"oscBSemitone", 19.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 3.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "ice", 5, PresetCategory::Pad },

        // 8. Rust Machine -- ancient, corroded mechanical device grinding back to
        //    life. Industrial, metallic, percussive.
        //    Box-Cyl subtraction -> machine housing with bore. Ring mod clang.
        { "Rust Machine", {
            {"shape1", 1}, {"shape2", 3}, {"operation", 3}, {"smoothK", 0.15f},
            {"size1", 0.5f}, {"size2", 0.3f}, {"offsetX", 0.15f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.45f}, {"scanHeight", 0.05f},
            {"topoMorph", 0.5f}, {"distScale", 4.5f}, {"scanMode", 2.f},
            {"attack", 0.001f}, {"decay", 0.3f}, {"sustain", 0.1f}, {"release", 0.8f},
            {"masterGain", 0.2f}, {"filterCutoff", 12000.f}, {"filterRes", 0.2f},
            {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 3.f},
            {"unisonSpread", 30.f}, {"unisonBlend", 0.8f},
            {"oscFold", 0.4f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.45f}, {"oscBSemitone", -5.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.12f},
            {"noiseType", 2.f}, {"noiseFilterCutoff", 2000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "rust", 2, PresetCategory::FX },

        // 9. Circuit Pulse -- digital signals racing through a circuit board, neon
        //    green on black. Tight, precise, electronic bass.
        //    Box+Box intersection -> PCB chip geometry. PW + sync for hard-edged bass.
        { "Circuit Pulse", {
            {"shape1", 1}, {"shape2", 1}, {"operation", 2}, {"smoothK", 0.1f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.2f}, {"offsetY", 0.1f},
            {"twist", 0.f}, {"scanRadius", 0.6f}, {"scanHeight", 0.f},
            {"topoMorph", 0.3f}, {"distScale", 5.f}, {"scanMode", 1.f},
            {"attack", 0.001f}, {"decay", 0.08f}, {"sustain", 0.9f}, {"release", 0.1f},
            {"masterGain", 0.25f}, {"filterCutoff", 5000.f}, {"filterRes", 0.4f},
            {"filterMode", 0.f},
            {"unisonVoices", 1.f}, {"unisonDetune", 20.f},
            {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.2f}, {"oscSync", 0.6f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 1.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "circuit", 0, PresetCategory::Bass },

        // 10. Scales of Leviathan -- enormous sea creature rising from the deep,
        //     iridescent scales catching dim light. Massive, otherworldly bass.
        //     Torus+Sphere smooth union -> serpentine body. Sub octave for seismic low.
        { "Scales of Leviathan", {
            {"shape1", 2}, {"shape2", 0}, {"operation", 0}, {"smoothK", 1.0f},
            {"size1", 0.55f}, {"size2", 0.45f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.5f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.9f}, {"distScale", 2.f}, {"scanMode", 0.f},
            {"attack", 0.05f}, {"decay", 0.2f}, {"sustain", 0.85f}, {"release", 0.6f},
            {"masterGain", 0.22f}, {"filterCutoff", 3500.f}, {"filterRes", 0.25f},
            {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 8.f},
            {"unisonSpread", 40.f}, {"unisonBlend", 0.65f},
            {"oscFold", 0.2f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.6f}, {"oscBSemitone", -12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.05f},
            {"noiseType", 2.f}, {"noiseFilterCutoff", 400.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "scales", 1, PresetCategory::Bass },

        // 11. Wood Spirit -- ancient tree spirit awakening in a misty forest.
        //     Warm, breathy, organic.
        //     Cyl+Torus smooth union -> gnarled trunk. AM tremolo, pink noise breath.
        { "Wood Spirit", {
            {"shape1", 3}, {"shape2", 2}, {"operation", 0}, {"smoothK", 0.8f},
            {"size1", 0.48f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.8f}, {"scanRadius", 0.45f}, {"scanHeight", 0.05f},
            {"topoMorph", 0.55f}, {"distScale", 3.f}, {"scanMode", 4.f},
            {"attack", 0.4f}, {"decay", 0.3f}, {"sustain", 0.75f}, {"release", 2.0f},
            {"masterGain", 0.2f}, {"filterCutoff", 2500.f}, {"filterRes", 0.3f},
            {"filterMode", 1.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 5.f},
            {"unisonSpread", 55.f}, {"unisonBlend", 0.7f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.25f}, {"oscBSemitone", 5.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 3.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.15f},
            {"noiseType", 1.f}, {"noiseFilterCutoff", 4000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "wood", 2, PresetCategory::FX },

        // 12. Organic Pulse -- bioluminescent organism pulsing in deep ocean darkness.
        //     Living, breathing, rhythmic.
        //     Sphere+Torus smooth union with twist -> organic blob. FM wobble, PD vocal.
        { "Organic Pulse", {
            {"shape1", 0}, {"shape2", 2}, {"operation", 0}, {"smoothK", 1.2f},
            {"size1", 0.5f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.6f}, {"scanRadius", 0.52f}, {"scanHeight", 0.f},
            {"topoMorph", 0.4f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.2f}, {"decay", 0.4f}, {"sustain", 0.8f}, {"release", 1.5f},
            {"masterGain", 0.2f}, {"filterCutoff", 6000.f}, {"filterRes", 0.12f},
            {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 6.f},
            {"unisonSpread", 45.f}, {"unisonBlend", 0.6f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.3f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.35f}, {"oscBSemitone", 3.f},
            {"oscBFine", 7.f}, {"oscBMixMode", 2.f}, {"oscBFMDepth", 0.2f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "organic", 1, PresetCategory::FX },

        // 13. Metal Forge -- white-hot metal hammered in a forge, sparks flying.
        //     Powerful, bright, percussive with sustain.
        //     Oct+Cyl intersection -> anvil. 8 unison, heavy fold, ring mod bell, 4x OS.
        { "Metal Forge", {
            {"shape1", 4}, {"shape2", 3}, {"operation", 2}, {"smoothK", 0.2f},
            {"size1", 0.5f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.42f}, {"scanHeight", 0.f},
            {"topoMorph", 0.6f}, {"distScale", 5.f}, {"scanMode", 2.f},
            {"attack", 0.001f}, {"decay", 0.6f}, {"sustain", 0.4f}, {"release", 1.5f},
            {"masterGain", 0.15f}, {"filterCutoff", 16000.f}, {"filterRes", 0.1f},
            {"filterMode", 0.f},
            {"unisonVoices", 8.f}, {"unisonDetune", 25.f},
            {"unisonSpread", 100.f}, {"unisonBlend", 0.45f},
            {"oscFold", 0.8f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.3f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 2.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "metal", 3, PresetCategory::Key },

        // =================================================================
        //  Phase 3 Presets -- showcasing LFO modulation
        // =================================================================

        // 14. LFO Wobble -- pulsing wobble bass with LFO1 on filter cutoff.
        //     Sphere+Box smooth union, triangle LFO sweeping the filter.
        { "LFO Wobble", {
            {"shape1", 0}, {"shape2", 1}, {"operation", 0}, {"smoothK", 0.6f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.01f}, {"decay", 0.1f}, {"sustain", 0.9f}, {"release", 0.3f},
            {"masterGain", 0.22f}, {"filterCutoff", 2000.f}, {"filterRes", 0.35f},
            {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 8.f},
            {"unisonSpread", 40.f}, {"unisonBlend", 0.6f},
            {"oscFold", 0.3f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 4.f}, {"lfo1Shape", 1.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "brick", 0, PresetCategory::Bass },

        // 15. Evolving Pad -- slow LFO1 on scan height + LFO2 on filter for drift.
        //     Torus+Sphere smooth union, sine LFOs create gradual timbral evolution.
        { "Evolving Pad", {
            {"shape1", 2}, {"shape2", 0}, {"operation", 0}, {"smoothK", 1.0f},
            {"size1", 0.5f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.3f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.6f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 1.0f}, {"decay", 0.5f}, {"sustain", 0.85f}, {"release", 3.0f},
            {"masterGain", 0.2f}, {"filterCutoff", 6000.f}, {"filterRes", 0.15f},
            {"filterMode", 0.f},
            {"unisonVoices", 5.f}, {"unisonDetune", 10.f},
            {"unisonSpread", 80.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.1f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.25f}, {"oscBSemitone", 12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 0.15f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.3f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 90.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "concrete", 1, PresetCategory::Pad },

        // =================================================================
        //  Phase 4 Presets -- showcasing FX engine
        // =================================================================

        // 16. Nebula Drift -- vast cosmic cloud slowly evolving. Reverb 0.8 + chorus,
        //     7 unison, slow attack. Sphere+Torus smooth union.
        { "Nebula Drift", {
            {"shape1", 0}, {"shape2", 2}, {"operation", 0}, {"smoothK", 0.9f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.4f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 1.2f}, {"decay", 0.5f}, {"sustain", 0.85f}, {"release", 4.0f},
            {"masterGain", 0.18f}, {"filterCutoff", 5000.f}, {"filterRes", 0.1f},
            {"filterMode", 0.f},
            {"unisonVoices", 7.f}, {"unisonDetune", 12.f},
            {"unisonSpread", 90.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.2f}, {"oscBSemitone", 12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 0.8f}, {"fxChorusDepth", 0.4f}, {"fxChorusMix", 0.35f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.8f}, {"fxReverbDamping", 0.4f}, {"fxReverbMix", 0.4f}
        }, "nebula", 4, PresetCategory::Pad },

        // 17. Aurora Veil -- shimmering northern lights curtain. LFO1->filter, chorus + reverb.
        //     Sphere+Box smooth union.
        { "Aurora Veil", {
            {"shape1", 0}, {"shape2", 1}, {"operation", 0}, {"smoothK", 1.0f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.8f}, {"decay", 0.6f}, {"sustain", 0.9f}, {"release", 3.5f},
            {"masterGain", 0.18f}, {"filterCutoff", 4000.f}, {"filterRes", 0.2f},
            {"filterMode", 0.f},
            {"unisonVoices", 5.f}, {"unisonDetune", 10.f},
            {"unisonSpread", 80.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.25f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 3.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 0.5f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.2f}, {"lfo2Shape", 1.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 1.2f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.7f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.35f}
        }, "aurora", 5, PresetCategory::Pad },

        // 18. Deep Space -- infinite void, echoing signals. Long delay + reverb, LFO2->scanHeight.
        //     Oct+Sphere smooth union with twist.
        { "Deep Space", {
            {"shape1", 4}, {"shape2", 0}, {"operation", 0}, {"smoothK", 0.8f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.3f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 1.5f}, {"decay", 0.8f}, {"sustain", 0.8f}, {"release", 5.0f},
            {"masterGain", 0.16f}, {"filterCutoff", 3000.f}, {"filterRes", 0.1f},
            {"filterMode", 0.f},
            {"unisonVoices", 6.f}, {"unisonDetune", 8.f},
            {"unisonSpread", 85.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.15f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 0.1f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.08f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 500.f}, {"fxDelayFeedback", 0.6f}, {"fxDelayMix", 0.4f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.9f}, {"fxReverbDamping", 0.3f}, {"fxReverbMix", 0.5f}
        }, "obsidian", 4, PresetCategory::Pad },

        // 19. Screaming Edge -- aggressive lead with soft clip dist + delay, fold+sync.
        //     Torus+Oct union with heavy twist.
        { "Screaming Edge", {
            {"shape1", 2}, {"shape2", 4}, {"operation", 1}, {"smoothK", 0.4f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 1.5f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.002f}, {"decay", 0.1f}, {"sustain", 0.9f}, {"release", 0.2f},
            {"masterGain", 0.2f}, {"filterCutoff", 12000.f}, {"filterRes", 0.3f},
            {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 15.f},
            {"unisonSpread", 60.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.6f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.5f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 8.f}, {"fxDistMix", 0.7f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 300.f}, {"fxDelayFeedback", 0.35f}, {"fxDelayMix", 0.25f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "plasma", 2, PresetCategory::Lead },

        // 20. Neon Razor -- sharp digital lead. Bitcrush, PW narrow, hard sync.
        //     Box+Cyl intersection.
        { "Neon Razor", {
            {"shape1", 1}, {"shape2", 3}, {"operation", 2}, {"smoothK", 0.2f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.001f}, {"decay", 0.08f}, {"sustain", 0.95f}, {"release", 0.15f},
            {"masterGain", 0.18f}, {"filterCutoff", 15000.f}, {"filterRes", 0.25f},
            {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 20.f},
            {"unisonSpread", 70.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.15f}, {"oscSync", 0.7f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 5.f}, {"fxDistMix", 0.5f}, {"fxDistType", 3.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "circuit", 0, PresetCategory::Lead },

        // 21. Seismic Drop -- heavy sub bass with fold dist, sub octave OscB.
        //     Sphere+Box smooth union.
        { "Seismic Drop", {
            {"shape1", 0}, {"shape2", 1}, {"operation", 0}, {"smoothK", 0.7f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.01f}, {"decay", 0.12f}, {"sustain", 0.85f}, {"release", 0.25f},
            {"masterGain", 0.22f}, {"filterCutoff", 4000.f}, {"filterRes", 0.35f},
            {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 6.f},
            {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.5f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.6f}, {"oscBSemitone", -12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 6.f}, {"fxDistMix", 0.6f}, {"fxDistType", 2.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "lava", 2, PresetCategory::Bass },

        // 22. Acid Line -- squelchy acid bass with LFO1 triangle->filter, high resonance.
        //     Torus+Cyl smooth union with twist.
        { "Acid Line", {
            {"shape1", 2}, {"shape2", 3}, {"operation", 0}, {"smoothK", 0.6f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.5f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.005f}, {"decay", 0.15f}, {"sustain", 0.8f}, {"release", 0.2f},
            {"masterGain", 0.22f}, {"filterCutoff", 1500.f}, {"filterRes", 0.55f},
            {"filterMode", 0.f},
            {"unisonVoices", 1.f}, {"unisonDetune", 8.f},
            {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.3f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 6.f}, {"lfo1Shape", 1.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 3.f}, {"fxDistMix", 0.4f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "alien", 0, PresetCategory::Bass },

        // 23. Glass Piano -- crystalline key sound. Chorus + reverb, OscB +12 additive.
        //     Cyl+Sphere smooth union.
        { "Glass Piano", {
            {"shape1", 3}, {"shape2", 0}, {"operation", 0}, {"smoothK", 0.5f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.01f}, {"decay", 0.5f}, {"sustain", 0.6f}, {"release", 1.5f},
            {"masterGain", 0.22f}, {"filterCutoff", 14000.f}, {"filterRes", 0.05f},
            {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 5.f},
            {"unisonSpread", 40.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.3f}, {"oscBSemitone", 12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 0.8f}, {"fxChorusDepth", 0.25f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.6f}, {"fxReverbMix", 0.3f}
        }, "crystal", 5, PresetCategory::Key },

        // 24. Warm Rhodes -- electric piano warmth. AM OscB, chorus + light reverb.
        //     Sphere+Torus smooth union.
        { "Warm Rhodes", {
            {"shape1", 0}, {"shape2", 2}, {"operation", 0}, {"smoothK", 0.8f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.01f}, {"decay", 0.3f}, {"sustain", 0.7f}, {"release", 0.8f},
            {"masterGain", 0.22f}, {"filterCutoff", 8000.f}, {"filterRes", 0.1f},
            {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 4.f},
            {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.3f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 3.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.2f}, {"fxChorusMix", 0.25f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.3f}, {"fxReverbDamping", 0.6f}, {"fxReverbMix", 0.2f}
        }, "wood", 2, PresetCategory::Key },

        // 25. Glitch Riser -- chaotic rising effect. S&H LFO->filter, bitcrush + delay.
        //     Box+Oct subtraction with heavy twist.
        { "Glitch Riser", {
            {"shape1", 1}, {"shape2", 4}, {"operation", 3}, {"smoothK", 0.3f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 2.0f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.3f}, {"decay", 0.4f}, {"sustain", 0.7f}, {"release", 2.0f},
            {"masterGain", 0.18f}, {"filterCutoff", 6000.f}, {"filterRes", 0.4f},
            {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 25.f},
            {"unisonSpread", 90.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.3f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 8.f}, {"lfo1Shape", 4.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 4.f}, {"fxDistMix", 0.5f}, {"fxDistType", 3.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 200.f}, {"fxDelayFeedback", 0.5f}, {"fxDelayMix", 0.35f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "circuit", 4, PresetCategory::FX },

        // 26. Void Echo -- cavernous echoing texture. Long delay + large reverb.
        //     Sphere+Oct smooth union with twist.
        { "Void Echo", {
            {"shape1", 0}, {"shape2", 4}, {"operation", 0}, {"smoothK", 1.0f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 1.0f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.5f}, {"decay", 0.6f}, {"sustain", 0.8f}, {"release", 3.0f},
            {"masterGain", 0.16f}, {"filterCutoff", 4000.f}, {"filterRes", 0.15f},
            {"filterMode", 0.f},
            {"unisonVoices", 5.f}, {"unisonDetune", 10.f},
            {"unisonSpread", 75.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.2f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 700.f}, {"fxDelayFeedback", 0.7f}, {"fxDelayMix", 0.5f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.95f}, {"fxReverbDamping", 0.2f}, {"fxReverbMix", 0.6f}
        }, "obsidian", 1, PresetCategory::FX },

        // 27. Circuit Bend -- mangled digital destruction. Heavy fold + chorus, twist.
        //     Box+Box intersection with OscB ring mod.
        { "Circuit Bend", {
            {"shape1", 1}, {"shape2", 1}, {"operation", 2}, {"smoothK", 0.15f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 3.0f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.01f}, {"decay", 0.2f}, {"sustain", 0.7f}, {"release", 0.5f},
            {"masterGain", 0.18f}, {"filterCutoff", 10000.f}, {"filterRes", 0.2f},
            {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 20.f},
            {"unisonSpread", 60.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.7f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.3f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.4f}, {"oscBSemitone", -7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 10.f}, {"fxDistMix", 0.6f}, {"fxDistType", 2.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 2.f}, {"fxChorusDepth", 0.5f}, {"fxChorusMix", 0.4f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "rust", 0, PresetCategory::FX },

        // 28. SDF Kick -- punchy kick drum. Fast attack/decay, fold dist, low filter.
        //     Sphere+Box smooth union, sub octave OscB.
        { "SDF Kick", {
            {"shape1", 0}, {"shape2", 1}, {"operation", 0}, {"smoothK", 0.5f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.001f}, {"decay", 0.1f}, {"sustain", 0.0f}, {"release", 0.15f},
            {"masterGain", 0.25f}, {"filterCutoff", 2000.f}, {"filterRes", 0.3f},
            {"filterMode", 0.f},
            {"unisonVoices", 1.f}, {"unisonDetune", 8.f},
            {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.4f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", -12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 5.f}, {"fxDistMix", 0.5f}, {"fxDistType", 2.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "obsidian", 0, PresetCategory::Perc },

        // 29. Metal Hit -- metallic percussion. Ring mod OscB, bright, fast envelope.
        //     Oct+Cyl intersection.
        { "Metal Hit", {
            {"shape1", 4}, {"shape2", 3}, {"operation", 2}, {"smoothK", 0.2f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.001f}, {"decay", 0.4f}, {"sustain", 0.05f}, {"release", 0.6f},
            {"masterGain", 0.2f}, {"filterCutoff", 16000.f}, {"filterRes", 0.15f},
            {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 15.f},
            {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.4f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.25f}
        }, "metal", 3, PresetCategory::Perc },

        // 30. Noise Burst -- short noise burst percussion. White noise, hard clip, fast envelope.
        //     Sphere+Sphere union.
        { "Noise Burst", {
            {"shape1", 0}, {"shape2", 0}, {"operation", 1}, {"smoothK", 0.5f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.001f}, {"decay", 0.05f}, {"sustain", 0.0f}, {"release", 0.1f},
            {"masterGain", 0.22f}, {"filterCutoff", 8000.f}, {"filterRes", 0.2f},
            {"filterMode", 0.f},
            {"unisonVoices", 1.f}, {"unisonDetune", 8.f},
            {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.5f},
            {"noiseType", 0.f}, {"noiseFilterCutoff", 12000.f},
            {"oversample", 0.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 8.f}, {"fxDistMix", 0.7f}, {"fxDistType", 1.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "rust", 0, PresetCategory::Perc }
    };
}
