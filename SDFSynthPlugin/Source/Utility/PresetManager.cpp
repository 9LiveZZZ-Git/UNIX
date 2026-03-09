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
        //  30 Factory Presets — all params set to prevent leakage.
        //  Uses all scan modes, new SDF primitives, new CSG operations,
        //  onion shell, SuperFormula, and creative FX combos.
        // =================================================================

        // ── PADS ─────────────────────────────────────────────────────

        // 1. Obsidian Monolith — massive dark angular drone.
        { "Obsidian Monolith", {
            {"shape1", 4}, {"shape2", 1}, {"operation", 2}, {"smoothK", 0.3f},
            {"size1", 0.55f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.45f}, {"scanHeight", -0.1f},
            {"topoMorph", 0.6f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.8f}, {"decay", 0.4f}, {"sustain", 0.9f}, {"release", 3.0f},
            {"masterGain", 0.2f}, {"filterCutoff", 2800.f}, {"filterRes", 0.18f}, {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 8.f}, {"unisonSpread", 40.f}, {"unisonBlend", 0.7f},
            {"oscFold", 0.15f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.06f}, {"noiseType", 2.f}, {"noiseFilterCutoff", 600.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 0.2f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.15f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.75f}, {"fxReverbDamping", 0.4f}, {"fxReverbMix", 0.35f}
        }, "obsidian", 0, PresetCategory::Pad },

        // 2. Crystal Cavern — HexPrism+Torus82 smooth intersect. Granular scan. Chorus+reverb.
        { "Crystal Cavern", {
            {"shape1", 8}, {"shape2", 9}, {"operation", 4}, {"smoothK", 0.25f},
            {"size1", 0.5f}, {"size2", 0.38f}, {"offsetX", 0.1f}, {"offsetY", 0.f},
            {"twist", 0.3f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.85f}, {"distScale", 5.5f}, {"scanMode", 3.f},
            {"attack", 0.01f}, {"decay", 0.8f}, {"sustain", 0.3f}, {"release", 2.5f},
            {"masterGain", 0.2f}, {"filterCutoff", 14000.f}, {"filterRes", 0.08f}, {"filterMode", 0.f},
            {"unisonVoices", 5.f}, {"unisonDetune", 14.f}, {"unisonSpread", 90.f}, {"unisonBlend", 0.45f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.2f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.2f}, {"oscBSemitone", 19.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 3.f}, {"oscBScanHeight", 0.2f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 0.3f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.5f}, {"lfo2Shape", 1.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 90.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 0.7f}, {"fxChorusDepth", 0.35f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.65f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.35f}
        }, "crystal", 5, PresetCategory::Pad },

        // 3. Nebula Drift — SuperFormula+Sphere smooth union. Spectral scan. 7 unison. Chorus+reverb.
        { "Nebula Drift", {
            {"shape1", 11}, {"shape2", 0}, {"operation", 0}, {"smoothK", 0.9f},
            {"size1", 0.48f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.4f}, {"scanRadius", 0.55f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.5f}, {"scanMode", 4.f},
            {"attack", 1.2f}, {"decay", 0.5f}, {"sustain", 0.85f}, {"release", 4.0f},
            {"masterGain", 0.18f}, {"filterCutoff", 5500.f}, {"filterRes", 0.1f}, {"filterMode", 0.f},
            {"unisonVoices", 7.f}, {"unisonDetune", 12.f}, {"unisonSpread", 92.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.2f}, {"oscBSemitone", 12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 4.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 8.f}, {"sfN1", 0.6f}, {"sfN2", 2.5f}, {"sfN3", 2.5f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 0.12f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.08f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 45.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 0.6f}, {"fxChorusDepth", 0.4f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 400.f}, {"fxDelayFeedback", 0.5f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.85f}, {"fxReverbDamping", 0.35f}, {"fxReverbMix", 0.45f}
        }, "nebula", 4, PresetCategory::Pad },

        // 4. Frozen Tundra — Sphere+RoundBox chamfer union. Contour scan. 7 unison. Chorus+reverb.
        { "Frozen Tundra", {
            {"shape1", 0}, {"shape2", 7}, {"operation", 6}, {"smoothK", 0.8f},
            {"size1", 0.5f}, {"size2", 0.38f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.05f},
            {"topoMorph", 0.8f}, {"distScale", 2.5f}, {"scanMode", 0.f},
            {"attack", 1.5f}, {"decay", 0.6f}, {"sustain", 0.85f}, {"release", 4.0f},
            {"masterGain", 0.18f}, {"filterCutoff", 6500.f}, {"filterRes", 0.1f}, {"filterMode", 0.f},
            {"unisonVoices", 7.f}, {"unisonDetune", 10.f}, {"unisonSpread", 95.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.12f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.18f}, {"oscBSemitone", 19.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 3.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 0.15f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.1f}, {"lfo2Shape", 1.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 0.5f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.25f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.8f}, {"fxReverbDamping", 0.3f}, {"fxReverbMix", 0.4f}
        }, "ice", 5, PresetCategory::Pad },

        // 5. Deep Space — Capsule+Oct smooth subtract. Acoustic scan. Delay+reverb.
        { "Deep Space", {
            {"shape1", 6}, {"shape2", 4}, {"operation", 5}, {"smoothK", 0.7f},
            {"size1", 0.5f}, {"size2", 0.35f}, {"offsetX", 0.15f}, {"offsetY", 0.f},
            {"twist", 0.3f}, {"scanRadius", 0.48f}, {"scanHeight", 0.f},
            {"topoMorph", 0.75f}, {"distScale", 3.5f}, {"scanMode", 2.f},
            {"attack", 1.5f}, {"decay", 0.8f}, {"sustain", 0.8f}, {"release", 5.0f},
            {"masterGain", 0.16f}, {"filterCutoff", 3200.f}, {"filterRes", 0.1f}, {"filterMode", 0.f},
            {"unisonVoices", 6.f}, {"unisonDetune", 8.f}, {"unisonSpread", 85.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.15f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.15f}, {"oscBSemitone", 7.f},
            {"oscBFine", 5.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 2.f}, {"oscBScanHeight", 0.1f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.03f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 8000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 0.1f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.07f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 550.f}, {"fxDelayFeedback", 0.6f}, {"fxDelayMix", 0.4f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.92f}, {"fxReverbDamping", 0.25f}, {"fxReverbMix", 0.5f}
        }, "alien", 1, PresetCategory::Pad },

        // 6. Aurora Veil — Torus88+Sphere onion shell. Ray March scan. Chorus+reverb.
        { "Aurora Veil", {
            {"shape1", 10}, {"shape2", 0}, {"operation", 0}, {"smoothK", 0.6f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.5f}, {"scanRadius", 0.52f}, {"scanHeight", 0.f},
            {"topoMorph", 0.65f}, {"distScale", 4.f}, {"scanMode", 1.f},
            {"attack", 0.8f}, {"decay", 0.6f}, {"sustain", 0.9f}, {"release", 3.5f},
            {"masterGain", 0.18f}, {"filterCutoff", 4500.f}, {"filterRes", 0.15f}, {"filterMode", 0.f},
            {"unisonVoices", 5.f}, {"unisonDetune", 11.f}, {"unisonSpread", 82.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.22f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 3.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 1.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 1.f}, {"onionThickness", 0.06f}, {"stairCount", 4.f},
            {"lfo1Rate", 0.4f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.2f}, {"lfo2Shape", 1.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 1.0f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.72f}, {"fxReverbDamping", 0.45f}, {"fxReverbMix", 0.38f}
        }, "aurora", 5, PresetCategory::Pad },

        // ── LEADS ────────────────────────────────────────────────────

        // 7. Plasma Storm — Torus+Oct union, twist. Ray March. Fold+sync, dist+delay.
        { "Plasma Storm", {
            {"shape1", 2}, {"shape2", 4}, {"operation", 1}, {"smoothK", 0.5f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.3f}, {"offsetY", 0.f},
            {"twist", 1.8f}, {"scanRadius", 0.55f}, {"scanHeight", 0.f},
            {"topoMorph", 0.95f}, {"distScale", 5.f}, {"scanMode", 1.f},
            {"attack", 0.002f}, {"decay", 0.05f}, {"sustain", 0.95f}, {"release", 0.15f},
            {"masterGain", 0.18f}, {"filterCutoff", 12000.f}, {"filterRes", 0.3f}, {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 18.f}, {"unisonSpread", 75.f}, {"unisonBlend", 0.55f},
            {"oscFold", 0.55f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.4f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.04f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 14000.f},
            {"oversample", 1.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 6.f}, {"lfo1Shape", 3.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 6.f}, {"fxDistMix", 0.6f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 280.f}, {"fxDelayFeedback", 0.3f}, {"fxDelayMix", 0.2f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "plasma", 2, PresetCategory::Lead },

        // 8. Screaming Edge — SuperFormula+Capsule smooth intersect. Contour. Dist+delay.
        { "Screaming Edge", {
            {"shape1", 11}, {"shape2", 6}, {"operation", 4}, {"smoothK", 0.4f},
            {"size1", 0.45f}, {"size2", 0.3f}, {"offsetX", 0.2f}, {"offsetY", 0.f},
            {"twist", 1.2f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.75f}, {"distScale", 4.f}, {"scanMode", 0.f},
            {"attack", 0.002f}, {"decay", 0.1f}, {"sustain", 0.9f}, {"release", 0.2f},
            {"masterGain", 0.2f}, {"filterCutoff", 10000.f}, {"filterRes", 0.25f}, {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 15.f}, {"unisonSpread", 60.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.6f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.5f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 5.f}, {"sfN1", 0.3f}, {"sfN2", 0.3f}, {"sfN3", 0.3f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 6.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 8.f}, {"fxDistMix", 0.7f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 300.f}, {"fxDelayFeedback", 0.35f}, {"fxDelayMix", 0.25f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "lava", 2, PresetCategory::Lead },

        // 9. Neon Razor — Box+Cylinder pipe. Ray March. Bitcrush, PW narrow, sync.
        { "Neon Razor", {
            {"shape1", 1}, {"shape2", 3}, {"operation", 10}, {"smoothK", 0.2f},
            {"size1", 0.42f}, {"size2", 0.32f}, {"offsetX", 0.25f}, {"offsetY", 0.1f},
            {"twist", 0.f}, {"scanRadius", 0.55f}, {"scanHeight", 0.05f},
            {"topoMorph", 0.5f}, {"distScale", 4.5f}, {"scanMode", 1.f},
            {"attack", 0.001f}, {"decay", 0.08f}, {"sustain", 0.95f}, {"release", 0.15f},
            {"masterGain", 0.2f}, {"filterCutoff", 14000.f}, {"filterRes", 0.2f}, {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 20.f}, {"unisonSpread", 70.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.15f}, {"oscSync", 0.7f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 5.f}, {"fxDistMix", 0.5f}, {"fxDistType", 3.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 180.f}, {"fxDelayFeedback", 0.3f}, {"fxDelayMix", 0.2f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "circuit", 0, PresetCategory::Lead },

        // 10. Alien Artifact — Sphere+Oct smooth union, twist. Spectral. FM OscB, phase dist.
        { "Alien Artifact", {
            {"shape1", 0}, {"shape2", 4}, {"operation", 0}, {"smoothK", 0.7f},
            {"size1", 0.48f}, {"size2", 0.38f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 2.5f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.5f}, {"scanMode", 4.f},
            {"attack", 0.3f}, {"decay", 0.5f}, {"sustain", 0.8f}, {"release", 1.5f},
            {"masterGain", 0.2f}, {"filterCutoff", 8000.f}, {"filterRes", 0.15f}, {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 18.f}, {"unisonSpread", 70.f}, {"unisonBlend", 0.4f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.4f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.4f}, {"oscBSemitone", 7.f},
            {"oscBFine", 3.f}, {"oscBMixMode", 2.f}, {"oscBFMDepth", 0.35f},
            {"oscBScanMode", 4.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 2.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.5f}, {"lfo2Shape", 2.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 1.5f}, {"fxChorusDepth", 0.2f}, {"fxChorusMix", 0.2f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 350.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.25f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "alien", 4, PresetCategory::Lead },

        // 11. Laser Blade — Torus82+HexPrism chamfer subtract. Ray March. PW narrow.
        { "Laser Blade", {
            {"shape1", 9}, {"shape2", 8}, {"operation", 8}, {"smoothK", 0.3f},
            {"size1", 0.45f}, {"size2", 0.3f}, {"offsetX", 0.15f}, {"offsetY", 0.f},
            {"twist", 0.8f}, {"scanRadius", 0.48f}, {"scanHeight", 0.1f},
            {"topoMorph", 0.6f}, {"distScale", 5.f}, {"scanMode", 1.f},
            {"attack", 0.001f}, {"decay", 0.06f}, {"sustain", 0.92f}, {"release", 0.12f},
            {"masterGain", 0.22f}, {"filterCutoff", 9000.f}, {"filterRes", 0.35f}, {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 6.f}, {"unisonSpread", 30.f}, {"unisonBlend", 0.7f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.2f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 5.f}, {"lfo1Shape", 2.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 220.f}, {"fxDelayFeedback", 0.35f}, {"fxDelayMix", 0.2f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "metal", 3, PresetCategory::Lead },

        // ── BASS ─────────────────────────────────────────────────────

        // 12. Lava Flow — Torus+Cylinder smooth union, twist. Ray March. Fold, ring mod, 2x OS.
        { "Lava Flow", {
            {"shape1", 2}, {"shape2", 3}, {"operation", 0}, {"smoothK", 0.8f},
            {"size1", 0.5f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 1.2f}, {"scanRadius", 0.55f}, {"scanHeight", 0.05f},
            {"topoMorph", 0.9f}, {"distScale", 4.f}, {"scanMode", 1.f},
            {"attack", 0.01f}, {"decay", 0.15f}, {"sustain", 0.85f}, {"release", 0.3f},
            {"masterGain", 0.2f}, {"filterCutoff", 6000.f}, {"filterRes", 0.25f}, {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 5.f}, {"unisonSpread", 55.f}, {"unisonBlend", 0.6f},
            {"oscFold", 0.7f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.3f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 1.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.08f}, {"noiseType", 1.f}, {"noiseFilterCutoff", 5000.f},
            {"oversample", 1.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 4.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "lava", 2, PresetCategory::Bass },

        // 13. Seismic Drop — Sphere+RoundBox smooth union. Contour. Sub OscB, fold dist.
        { "Seismic Drop", {
            {"shape1", 0}, {"shape2", 7}, {"operation", 0}, {"smoothK", 0.7f},
            {"size1", 0.55f}, {"size2", 0.42f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.48f}, {"scanHeight", 0.f},
            {"topoMorph", 0.8f}, {"distScale", 2.5f}, {"scanMode", 0.f},
            {"attack", 0.01f}, {"decay", 0.12f}, {"sustain", 0.85f}, {"release", 0.25f},
            {"masterGain", 0.22f}, {"filterCutoff", 3500.f}, {"filterRes", 0.3f}, {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 6.f}, {"unisonSpread", 40.f}, {"unisonBlend", 0.6f},
            {"oscFold", 0.5f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.6f}, {"oscBSemitone", -12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 6.f}, {"fxDistMix", 0.55f}, {"fxDistType", 2.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "obsidian", 0, PresetCategory::Bass },

        // 14. Acid Line — Torus88+Cylinder chamfer union, twist. Acoustic. LFO triangle, dist.
        { "Acid Line", {
            {"shape1", 10}, {"shape2", 3}, {"operation", 6}, {"smoothK", 0.5f},
            {"size1", 0.48f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.5f}, {"scanRadius", 0.52f}, {"scanHeight", 0.f},
            {"topoMorph", 0.6f}, {"distScale", 3.5f}, {"scanMode", 2.f},
            {"attack", 0.005f}, {"decay", 0.15f}, {"sustain", 0.8f}, {"release", 0.2f},
            {"masterGain", 0.22f}, {"filterCutoff", 1500.f}, {"filterRes", 0.55f}, {"filterMode", 0.f},
            {"unisonVoices", 1.f}, {"unisonDetune", 8.f}, {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.3f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 6.f}, {"lfo1Shape", 1.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 3.f}, {"fxDistMix", 0.4f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "scales", 0, PresetCategory::Bass },

        // 15. Circuit Pulse — Box+Box intersection. Granular. PW+sync, 2x OS.
        { "Circuit Pulse", {
            {"shape1", 1}, {"shape2", 1}, {"operation", 2}, {"smoothK", 0.1f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.2f}, {"offsetY", 0.1f},
            {"twist", 0.f}, {"scanRadius", 0.6f}, {"scanHeight", 0.f},
            {"topoMorph", 0.3f}, {"distScale", 5.f}, {"scanMode", 3.f},
            {"attack", 0.001f}, {"decay", 0.08f}, {"sustain", 0.9f}, {"release", 0.1f},
            {"masterGain", 0.25f}, {"filterCutoff", 5000.f}, {"filterRes", 0.4f}, {"filterMode", 0.f},
            {"unisonVoices", 1.f}, {"unisonDetune", 20.f}, {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.2f}, {"oscSync", 0.6f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 1.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "circuit", 0, PresetCategory::Bass },

        // 16. LFO Wobble — SuperFormula+Box smooth subtract. Contour. Wavefolding, LFO triangle.
        { "LFO Wobble", {
            {"shape1", 11}, {"shape2", 1}, {"operation", 5}, {"smoothK", 0.6f},
            {"size1", 0.5f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.01f}, {"decay", 0.1f}, {"sustain", 0.9f}, {"release", 0.3f},
            {"masterGain", 0.22f}, {"filterCutoff", 2000.f}, {"filterRes", 0.4f}, {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 8.f}, {"unisonSpread", 40.f}, {"unisonBlend", 0.6f},
            {"oscFold", 0.3f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 7.f}, {"sfN1", 0.4f}, {"sfN2", 1.5f}, {"sfN3", 1.5f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 4.f}, {"lfo1Shape", 1.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "brick", 0, PresetCategory::Bass },

        // ── KEYS ─────────────────────────────────────────────────────

        // 17. Marble Cathedral — Cyl+Sphere smooth union. Spectral. OscB +12, reverb.
        { "Marble Cathedral", {
            {"shape1", 3}, {"shape2", 0}, {"operation", 0}, {"smoothK", 1.0f},
            {"size1", 0.5f}, {"size2", 0.45f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.2f}, {"scanRadius", 0.48f}, {"scanHeight", 0.f},
            {"topoMorph", 0.65f}, {"distScale", 3.5f}, {"scanMode", 4.f},
            {"attack", 0.15f}, {"decay", 0.1f}, {"sustain", 0.92f}, {"release", 1.8f},
            {"masterGain", 0.22f}, {"filterCutoff", 10000.f}, {"filterRes", 0.08f}, {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 6.f}, {"unisonSpread", 50.f}, {"unisonBlend", 0.7f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.42f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.35f}, {"oscBSemitone", 12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 4.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.7f}, {"fxReverbDamping", 0.4f}, {"fxReverbMix", 0.35f}
        }, "marble", 5, PresetCategory::Key },

        // 18. Glass Piano — Capsule+Torus smooth intersect. Granular. OscB +12, chorus+reverb.
        { "Glass Piano", {
            {"shape1", 6}, {"shape2", 2}, {"operation", 4}, {"smoothK", 0.35f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.1f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.46f}, {"scanHeight", 0.f},
            {"topoMorph", 0.55f}, {"distScale", 4.f}, {"scanMode", 3.f},
            {"attack", 0.01f}, {"decay", 0.5f}, {"sustain", 0.55f}, {"release", 1.5f},
            {"masterGain", 0.22f}, {"filterCutoff", 14000.f}, {"filterRes", 0.05f}, {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 5.f}, {"unisonSpread", 40.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.28f}, {"oscBSemitone", 12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 3.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 0.8f}, {"fxChorusDepth", 0.25f}, {"fxChorusMix", 0.28f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.45f}, {"fxReverbDamping", 0.6f}, {"fxReverbMix", 0.28f}
        }, "crystal", 5, PresetCategory::Key },

        // 19. Warm Rhodes — Sphere+Torus smooth union. Contour. AM OscB +7, chorus+reverb.
        { "Warm Rhodes", {
            {"shape1", 0}, {"shape2", 2}, {"operation", 0}, {"smoothK", 0.8f},
            {"size1", 0.48f}, {"size2", 0.38f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.5f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.01f}, {"decay", 0.35f}, {"sustain", 0.65f}, {"release", 0.8f},
            {"masterGain", 0.22f}, {"filterCutoff", 7000.f}, {"filterRes", 0.1f}, {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 4.f}, {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.1f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.3f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 3.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.2f}, {"fxChorusMix", 0.25f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.3f}, {"fxReverbDamping", 0.6f}, {"fxReverbMix", 0.2f}
        }, "wood", 3, PresetCategory::Key },

        // 20. Metal Forge — Oct+HexPrism intersection. Acoustic. 8 unison, fold, ring mod, 4x OS.
        { "Metal Forge", {
            {"shape1", 4}, {"shape2", 8}, {"operation", 2}, {"smoothK", 0.2f},
            {"size1", 0.5f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.42f}, {"scanHeight", 0.f},
            {"topoMorph", 0.6f}, {"distScale", 5.f}, {"scanMode", 2.f},
            {"attack", 0.001f}, {"decay", 0.6f}, {"sustain", 0.4f}, {"release", 1.5f},
            {"masterGain", 0.15f}, {"filterCutoff", 16000.f}, {"filterRes", 0.1f}, {"filterMode", 0.f},
            {"unisonVoices", 8.f}, {"unisonDetune", 25.f}, {"unisonSpread", 100.f}, {"unisonBlend", 0.45f},
            {"oscFold", 0.8f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.3f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 2.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.04f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 16000.f},
            {"oversample", 2.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.35f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.2f}
        }, "metal", 3, PresetCategory::Key },

        // 21. Hollow Bell — RoundBox+Torus82 onion pipe. Spectral. Ring mod, delay+reverb.
        { "Hollow Bell", {
            {"shape1", 7}, {"shape2", 9}, {"operation", 10}, {"smoothK", 0.3f},
            {"size1", 0.42f}, {"size2", 0.32f}, {"offsetX", 0.15f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.44f}, {"scanHeight", 0.f},
            {"topoMorph", 0.5f}, {"distScale", 4.5f}, {"scanMode", 4.f},
            {"attack", 0.001f}, {"decay", 0.8f}, {"sustain", 0.15f}, {"release", 2.0f},
            {"masterGain", 0.2f}, {"filterCutoff", 12000.f}, {"filterRes", 0.08f}, {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 8.f}, {"unisonSpread", 65.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.35f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 4.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 1.f}, {"onionThickness", 0.04f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 400.f}, {"fxDelayFeedback", 0.45f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.6f}, {"fxReverbDamping", 0.4f}, {"fxReverbMix", 0.35f}
        }, "marble", 5, PresetCategory::Key },

        // ── FX ───────────────────────────────────────────────────────

        // 22. Rust Machine — Box-Cyl subtraction. Acoustic. Ring mod, brown noise, dist+delay.
        { "Rust Machine", {
            {"shape1", 1}, {"shape2", 3}, {"operation", 3}, {"smoothK", 0.15f},
            {"size1", 0.5f}, {"size2", 0.3f}, {"offsetX", 0.15f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.45f}, {"scanHeight", 0.05f},
            {"topoMorph", 0.5f}, {"distScale", 4.5f}, {"scanMode", 2.f},
            {"attack", 0.001f}, {"decay", 0.3f}, {"sustain", 0.1f}, {"release", 0.8f},
            {"masterGain", 0.2f}, {"filterCutoff", 10000.f}, {"filterRes", 0.2f}, {"filterMode", 0.f},
            {"unisonVoices", 2.f}, {"unisonDetune", 3.f}, {"unisonSpread", 30.f}, {"unisonBlend", 0.8f},
            {"oscFold", 0.4f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.45f}, {"oscBSemitone", -5.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 2.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.12f}, {"noiseType", 2.f}, {"noiseFilterCutoff", 2000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 3.f}, {"lfo1Shape", 4.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 5.f}, {"fxDistMix", 0.5f}, {"fxDistType", 2.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 150.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.25f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "rust", 2, PresetCategory::FX },

        // 23. Glitch Riser — Box+Oct smooth subtract, twist. Ray March. S&H LFO, bitcrush+delay.
        { "Glitch Riser", {
            {"shape1", 1}, {"shape2", 4}, {"operation", 5}, {"smoothK", 0.3f},
            {"size1", 0.42f}, {"size2", 0.32f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 2.5f}, {"scanRadius", 0.55f}, {"scanHeight", 0.f},
            {"topoMorph", 0.8f}, {"distScale", 5.5f}, {"scanMode", 1.f},
            {"attack", 0.3f}, {"decay", 0.4f}, {"sustain", 0.7f}, {"release", 2.0f},
            {"masterGain", 0.18f}, {"filterCutoff", 6000.f}, {"filterRes", 0.4f}, {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 30.f}, {"unisonSpread", 90.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.3f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 8.f}, {"lfo1Shape", 4.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.5f}, {"lfo2Shape", 5.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 4.f}, {"fxDistMix", 0.5f}, {"fxDistType", 3.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 200.f}, {"fxDelayFeedback", 0.55f}, {"fxDelayMix", 0.35f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "circuit", 4, PresetCategory::FX },

        // 24. Void Echo — Sphere+Oct smooth union, twist. Granular. Long delay+massive reverb.
        { "Void Echo", {
            {"shape1", 0}, {"shape2", 4}, {"operation", 0}, {"smoothK", 1.0f},
            {"size1", 0.5f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 1.0f}, {"scanRadius", 0.52f}, {"scanHeight", -0.15f},
            {"topoMorph", 0.45f}, {"distScale", 3.5f}, {"scanMode", 3.f},
            {"attack", 0.5f}, {"decay", 0.6f}, {"sustain", 0.8f}, {"release", 3.0f},
            {"masterGain", 0.16f}, {"filterCutoff", 4000.f}, {"filterRes", 0.15f}, {"filterMode", 0.f},
            {"unisonVoices", 5.f}, {"unisonDetune", 10.f}, {"unisonSpread", 75.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.2f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 0.1f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.08f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 1.f}, {"fxDelayTime", 700.f}, {"fxDelayFeedback", 0.7f}, {"fxDelayMix", 0.5f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.95f}, {"fxReverbDamping", 0.2f}, {"fxReverbMix", 0.6f}
        }, "obsidian", 1, PresetCategory::FX },

        // 25. Circuit Bend — Box+Box intersection, twist. Contour. Fold+chorus, ring mod, fold dist.
        { "Circuit Bend", {
            {"shape1", 1}, {"shape2", 1}, {"operation", 2}, {"smoothK", 0.15f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.2f}, {"offsetY", 0.1f},
            {"twist", 3.0f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.4f}, {"distScale", 4.f}, {"scanMode", 0.f},
            {"attack", 0.01f}, {"decay", 0.2f}, {"sustain", 0.7f}, {"release", 0.5f},
            {"masterGain", 0.18f}, {"filterCutoff", 8000.f}, {"filterRes", 0.2f}, {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 22.f}, {"unisonSpread", 60.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.7f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.3f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.4f}, {"oscBSemitone", -7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 7.f}, {"lfo1Shape", 3.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 10.f}, {"fxDistMix", 0.6f}, {"fxDistType", 2.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 2.f}, {"fxChorusDepth", 0.5f}, {"fxChorusMix", 0.4f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "rust", 0, PresetCategory::FX },

        // 26. Organic Pulse — Sphere+Torus smooth union, twist. Contour. FM OscB, phase dist.
        { "Organic Pulse", {
            {"shape1", 0}, {"shape2", 2}, {"operation", 0}, {"smoothK", 1.2f},
            {"size1", 0.5f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.6f}, {"scanRadius", 0.52f}, {"scanHeight", 0.f},
            {"topoMorph", 0.4f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.2f}, {"decay", 0.4f}, {"sustain", 0.8f}, {"release", 1.5f},
            {"masterGain", 0.2f}, {"filterCutoff", 5500.f}, {"filterRes", 0.12f}, {"filterMode", 0.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 6.f}, {"unisonSpread", 45.f}, {"unisonBlend", 0.6f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.3f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.35f}, {"oscBSemitone", 3.f},
            {"oscBFine", 7.f}, {"oscBMixMode", 2.f}, {"oscBFMDepth", 0.25f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 2.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.3f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 90.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 1.5f}, {"fxChorusDepth", 0.25f}, {"fxChorusMix", 0.25f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "organic", 1, PresetCategory::FX },

        // 27. Wood Spirit — Cyl+Torus smooth union, twist. Acoustic. AM OscB, pink noise, BP filter.
        { "Wood Spirit", {
            {"shape1", 3}, {"shape2", 2}, {"operation", 0}, {"smoothK", 0.8f},
            {"size1", 0.48f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.8f}, {"scanRadius", 0.45f}, {"scanHeight", 0.05f},
            {"topoMorph", 0.55f}, {"distScale", 3.f}, {"scanMode", 2.f},
            {"attack", 0.4f}, {"decay", 0.3f}, {"sustain", 0.75f}, {"release", 2.0f},
            {"masterGain", 0.2f}, {"filterCutoff", 2500.f}, {"filterRes", 0.3f}, {"filterMode", 1.f},
            {"unisonVoices", 3.f}, {"unisonDetune", 5.f}, {"unisonSpread", 55.f}, {"unisonBlend", 0.7f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.25f}, {"oscBSemitone", 5.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 3.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 2.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.15f}, {"noiseType", 1.f}, {"noiseFilterCutoff", 4000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 0.8f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 0.4f}, {"lfo2Shape", 1.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 1.f}, {"fxChorusRate", 0.6f}, {"fxChorusDepth", 0.2f}, {"fxChorusMix", 0.2f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.55f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "wood", 2, PresetCategory::FX },

        // ── PERCUSSION ───────────────────────────────────────────────

        // 28. SDF Kick — Sphere+RoundBox smooth union. Contour. Sub OscB, fold dist.
        { "SDF Kick", {
            {"shape1", 0}, {"shape2", 7}, {"operation", 0}, {"smoothK", 0.5f},
            {"size1", 0.5f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 2.5f}, {"scanMode", 0.f},
            {"attack", 0.001f}, {"decay", 0.1f}, {"sustain", 0.0f}, {"release", 0.15f},
            {"masterGain", 0.25f}, {"filterCutoff", 2000.f}, {"filterRes", 0.3f}, {"filterMode", 0.f},
            {"unisonVoices", 1.f}, {"unisonDetune", 8.f}, {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.4f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", -12.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 1.f}, {"fxDistDrive", 5.f}, {"fxDistMix", 0.5f}, {"fxDistType", 2.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 0.f}, {"fxReverbSize", 0.5f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.3f}
        }, "obsidian", 0, PresetCategory::Perc },

        // 29. Metal Hit — Oct+Capsule chamfer intersect. Granular. Ring mod OscB, reverb.
        { "Metal Hit", {
            {"shape1", 4}, {"shape2", 6}, {"operation", 7}, {"smoothK", 0.2f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.1f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.48f}, {"scanHeight", 0.f},
            {"topoMorph", 0.6f}, {"distScale", 4.f}, {"scanMode", 3.f},
            {"attack", 0.001f}, {"decay", 0.4f}, {"sustain", 0.05f}, {"release", 0.6f},
            {"masterGain", 0.2f}, {"filterCutoff", 16000.f}, {"filterRes", 0.12f}, {"filterMode", 0.f},
            {"unisonVoices", 4.f}, {"unisonDetune", 15.f}, {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 1.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 7.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 1.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 3.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 0.f}, {"noiseLevel", 0.2f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 20000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
            {"lfo1Rate", 1.f}, {"lfo1Shape", 0.f}, {"lfo1Sync", 0.f}, {"lfo1Phase", 0.f},
            {"lfo2Rate", 1.f}, {"lfo2Shape", 0.f}, {"lfo2Sync", 0.f}, {"lfo2Phase", 0.f},
            {"macro1", 0.5f}, {"macro2", 0.5f}, {"macro3", 0.5f}, {"macro4", 0.5f},
            {"fxDistEnable", 0.f}, {"fxDistDrive", 1.f}, {"fxDistMix", 0.5f}, {"fxDistType", 0.f},
            {"fxChorusEnable", 0.f}, {"fxChorusRate", 1.f}, {"fxChorusDepth", 0.3f}, {"fxChorusMix", 0.3f},
            {"fxDelayEnable", 0.f}, {"fxDelayTime", 250.f}, {"fxDelayFeedback", 0.4f}, {"fxDelayMix", 0.3f},
            {"fxReverbEnable", 1.f}, {"fxReverbSize", 0.4f}, {"fxReverbDamping", 0.5f}, {"fxReverbMix", 0.25f}
        }, "metal", 3, PresetCategory::Perc },

        // 30. Noise Burst — Sphere+Sphere union. Ray March. White noise, hard clip, fast envelope.
        { "Noise Burst", {
            {"shape1", 0}, {"shape2", 0}, {"operation", 1}, {"smoothK", 0.5f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.7f}, {"distScale", 3.f}, {"scanMode", 1.f},
            {"attack", 0.001f}, {"decay", 0.05f}, {"sustain", 0.0f}, {"release", 0.1f},
            {"masterGain", 0.22f}, {"filterCutoff", 8000.f}, {"filterRes", 0.2f}, {"filterMode", 0.f},
            {"unisonVoices", 1.f}, {"unisonDetune", 8.f}, {"unisonSpread", 50.f}, {"unisonBlend", 0.5f},
            {"oscFold", 0.f}, {"oscPhaseDist", 0.f}, {"oscPW", 0.5f}, {"oscSync", 0.f},
            {"oscBEnable", 0.f}, {"oscBLevel", 0.5f}, {"oscBSemitone", 0.f},
            {"oscBFine", 0.f}, {"oscBMixMode", 0.f}, {"oscBFMDepth", 0.f},
            {"oscBScanMode", 0.f}, {"oscBScanHeight", 0.f},
            {"noiseEnable", 1.f}, {"noiseLevel", 0.5f}, {"noiseType", 0.f}, {"noiseFilterCutoff", 12000.f},
            {"oversample", 0.f},
            {"sfM", 6.f}, {"sfN1", 1.f}, {"sfN2", 1.f}, {"sfN3", 1.f},
            {"onionEnable", 0.f}, {"onionThickness", 0.05f}, {"stairCount", 4.f},
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
