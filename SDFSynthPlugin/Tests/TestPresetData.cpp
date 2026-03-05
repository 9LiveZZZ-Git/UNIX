#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Utility/PresetManager.h"
#include "Texture/ProceduralLibrary.h"
#include <set>
#include <map>

// Validates factory preset data: names, textures, skyboxes, param ranges,
// and coverage of Phase 2 features.

class PresetDataTest : public juce::UnitTest
{
public:
    PresetDataTest() : juce::UnitTest("PresetData") {}

    void runTest() override
    {
        auto presets = PresetManager::getFactoryPresets();

        // ── BASIC COUNT ────────────────────────────────────────────
        beginTest("Exactly 30 factory presets");
        {
            expectEquals(static_cast<int>(presets.size()), 30);
        }

        // ── UNIQUE NAMES ───────────────────────────────────────────
        beginTest("All preset names are unique");
        {
            std::set<juce::String> names;
            for (auto& p : presets)
                names.insert(p.name);
            expectEquals(static_cast<int>(names.size()), static_cast<int>(presets.size()),
                         "All names should be unique");
        }

        // ── VALID TEXTURES ────────────────────────────────────────
        beginTest("All presets have a valid texture key");
        {
            auto validKeys = ProceduralLibrary::getPresetKeys();
            for (auto& p : presets)
            {
                expect(!p.textureKey.isEmpty(),
                       "Preset '" + p.name + "' has empty texture key");
                expect(validKeys.contains(p.textureKey),
                       "Preset '" + p.name + "' has unknown texture key: " + p.textureKey);
            }
        }

        // ── VALID SKYBOX INDICES ───────────────────────────────────
        beginTest("All presets have valid skybox indices (0-5)");
        {
            bool allValid = true;
            for (auto& p : presets)
            {
                if (p.skyboxPreset < 0 || p.skyboxPreset > 5)
                {
                    allValid = false;
                    break;
                }
            }
            expect(allValid, "Every preset should have skyboxPreset 0-5 (no -1 unchanged)");
        }

        // ── ALL PRESETS SET PHASE 2 PARAMS ─────────────────────────
        beginTest("Every preset sets all Phase 2 parameters");
        {
            juce::StringArray phase2Params = {
                "unisonVoices", "unisonDetune", "unisonSpread", "unisonBlend",
                "oscFold", "oscPhaseDist", "oscPW", "oscSync",
                "oscBEnable", "oscBLevel", "oscBSemitone", "oscBFine",
                "oscBMixMode", "oscBFMDepth",
                "noiseEnable", "noiseLevel", "noiseType", "noiseFilterCutoff",
                "oversample", "filterMode"
            };

            for (auto& preset : presets)
            {
                std::set<juce::String> paramIds;
                for (auto& [id, val] : preset.params)
                    paramIds.insert(id);

                for (auto& required : phase2Params)
                {
                    expect(paramIds.count(required) > 0,
                           "Preset '" + preset.name + "' missing Phase 2 param: " + required);
                }
            }
        }

        // ── PARAMETER RANGE VALIDATION ─────────────────────────────
        beginTest("All parameter values within APVTS ranges");
        {
            // Define known ranges: paramId → {min, max}
            struct Range { float min, max; };
            std::map<juce::String, Range> ranges = {
                {"shape1",          {0.f, 5.f}},
                {"shape2",          {0.f, 5.f}},
                {"operation",       {0.f, 3.f}},
                {"smoothK",         {0.01f, 1.5f}},
                {"size1",           {0.1f, 0.7f}},
                {"size2",           {0.1f, 0.7f}},
                {"offsetX",         {-1.f, 1.f}},
                {"offsetY",         {-1.f, 1.f}},
                {"twist",           {0.f, 6.f}},
                {"scanRadius",      {0.1f, 1.5f}},
                {"scanHeight",      {-0.9f, 0.9f}},
                {"topoMorph",       {0.f, 1.f}},
                {"distScale",       {0.5f, 8.f}},
                {"scanMode",        {0.f, 5.f}},
                {"attack",          {0.001f, 2.f}},
                {"decay",           {0.001f, 2.f}},
                {"sustain",         {0.f, 1.f}},
                {"release",         {0.001f, 5.f}},
                {"masterGain",      {0.f, 1.f}},
                {"filterCutoff",    {20.f, 20000.f}},
                {"filterRes",       {0.f, 1.f}},
                {"filterMode",      {0.f, 4.f}},
                {"noiseEnable",     {0.f, 1.f}},
                {"noiseLevel",      {0.f, 1.f}},
                {"noiseType",       {0.f, 2.f}},
                {"noiseFilterCutoff", {20.f, 20000.f}},
                {"oscFold",         {0.f, 1.f}},
                {"oscPhaseDist",    {0.f, 1.f}},
                {"oscPW",           {0.f, 1.f}},
                {"oscSync",         {0.f, 1.f}},
                {"oscBEnable",      {0.f, 1.f}},
                {"oscBLevel",       {0.f, 1.f}},
                {"oscBSemitone",    {-24.f, 24.f}},
                {"oscBFine",        {-100.f, 100.f}},
                {"oscBMixMode",     {0.f, 3.f}},
                {"oscBFMDepth",     {0.f, 1.f}},
                {"unisonVoices",    {1.f, 8.f}},
                {"unisonDetune",    {0.f, 100.f}},
                {"unisonSpread",    {0.f, 100.f}},
                {"unisonBlend",     {0.f, 1.f}},
                {"oversample",      {0.f, 2.f}},
            };

            bool allInRange = true;
            juce::String failMsg;
            for (auto& preset : presets)
            {
                for (auto& [id, val] : preset.params)
                {
                    auto it = ranges.find(id);
                    if (it != ranges.end())
                    {
                        if (val < it->second.min - 0.001f || val > it->second.max + 0.001f)
                        {
                            failMsg = "Preset '" + preset.name + "' param '" + id
                                      + "' = " + juce::String(val)
                                      + " out of range [" + juce::String(it->second.min)
                                      + ", " + juce::String(it->second.max) + "]";
                            allInRange = false;
                            break;
                        }
                    }
                }
                if (!allInRange) break;
            }
            expect(allInRange, allInRange ? "" : failMsg);
        }

        // ── SCAN MODE COVERAGE ─────────────────────────────────────
        beginTest("All 6 scan modes used across presets");
        {
            std::set<int> modesUsed;
            for (auto& preset : presets)
                for (auto& [id, val] : preset.params)
                    if (id == "scanMode")
                        modesUsed.insert(static_cast<int>(val));

            expectEquals(static_cast<int>(modesUsed.size()), 6,
                         "All 6 scan modes (0-5) should appear in factory presets");
        }

        // ── PHASE 2 FEATURE COVERAGE ──────────────────────────────
        beginTest("Unison used in at least 10 presets");
        {
            int count = 0;
            for (auto& preset : presets)
                for (auto& [id, val] : preset.params)
                    if (id == "unisonVoices" && val > 1.f)
                        ++count;
            expect(count >= 10, "At least 10 presets should use unison >1 (got "
                   + juce::String(count) + ")");
        }

        beginTest("Wavefolding used in at least 5 presets");
        {
            int count = 0;
            for (auto& preset : presets)
                for (auto& [id, val] : preset.params)
                    if (id == "oscFold" && val > 0.01f)
                        ++count;
            expect(count >= 5, "At least 5 presets should use wavefolding (got "
                   + juce::String(count) + ")");
        }

        beginTest("Osc B used in at least 8 presets");
        {
            int count = 0;
            for (auto& preset : presets)
                for (auto& [id, val] : preset.params)
                    if (id == "oscBEnable" && val > 0.5f)
                        ++count;
            expect(count >= 8, "At least 8 presets should use Osc B (got "
                   + juce::String(count) + ")");
        }

        beginTest("Noise used in at least 6 presets");
        {
            int count = 0;
            for (auto& preset : presets)
                for (auto& [id, val] : preset.params)
                    if (id == "noiseEnable" && val > 0.5f)
                        ++count;
            expect(count >= 6, "At least 6 presets should use noise (got "
                   + juce::String(count) + ")");
        }

        beginTest("Oversampling used in at least 3 presets");
        {
            int count = 0;
            for (auto& preset : presets)
                for (auto& [id, val] : preset.params)
                    if (id == "oversample" && val > 0.5f)
                        ++count;
            expect(count >= 3, "At least 3 presets should use oversampling (got "
                   + juce::String(count) + ")");
        }

        beginTest("Phase distortion used in at least 3 presets");
        {
            int count = 0;
            for (auto& preset : presets)
                for (auto& [id, val] : preset.params)
                    if (id == "oscPhaseDist" && val > 0.01f)
                        ++count;
            expect(count >= 3, "At least 3 presets should use phase distortion (got "
                   + juce::String(count) + ")");
        }

        beginTest("At least one preset uses BP filter mode");
        {
            bool found = false;
            for (auto& preset : presets)
                for (auto& [id, val] : preset.params)
                    if (id == "filterMode" && static_cast<int>(val) == 1)
                        found = true;
            expect(found, "At least one preset should use BandPass filter mode");
        }
    }
};

static PresetDataTest presetDataTest;
