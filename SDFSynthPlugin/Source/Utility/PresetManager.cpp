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

std::vector<PresetManager::FactoryPreset> PresetManager::getFactoryPresets()
{
    return {
        // ── CONTOUR (Mode 0) ────────────────────────────────────────

        // 1. MRI Sphere — pure, clean sine-like tone from a sphere cross-section.
        //    Full MRI morph shows the smooth radial profile; high scanRadius
        //    catches the sphere's curvature. Unfiltered for clinical clarity.
        { "MRI Sphere", {
            {"shape1", 0}, {"shape2", 2}, {"operation", 0}, {"smoothK", 0.5f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.45f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.55f}, {"scanHeight", 0.f},
            {"topoMorph", 1.f}, {"distScale", 3.f}, {"scanMode", 0.f},
            {"attack", 0.01f}, {"decay", 0.12f}, {"sustain", 0.8f}, {"release", 0.4f},
            {"masterGain", 0.25f}, {"filterCutoff", 20000.f}, {"filterRes", 0.f}
        }, "", -1 },

        // 2. Carved Box — harsh, angular buzz from a box with a sphere carved out.
        //    The subtraction creates sharp corners in the cross-section that produce
        //    rich odd harmonics. Concrete texture adds grit, moderate filter.
        { "Carved Box", {
            {"shape1", 1}, {"shape2", 0}, {"operation", 3}, {"smoothK", 0.15f},
            {"size1", 0.45f}, {"size2", 0.32f}, {"offsetX", 0.2f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.42f}, {"scanHeight", 0.08f},
            {"topoMorph", 0.7f}, {"distScale", 5.f}, {"scanMode", 0.f},
            {"attack", 0.003f}, {"decay", 0.15f}, {"sustain", 0.65f}, {"release", 0.2f},
            {"masterGain", 0.22f}, {"filterCutoff", 8000.f}, {"filterRes", 0.25f}
        }, "concrete", -1 },

        // ── RAY MARCH SONIFY (Mode 1) ───────────────────────────────

        // 3. Ray Pluck — a single ray fires through a torus, hitting the surface
        //    creates a sharp nasal transient that dies away. Very fast attack/decay
        //    like a plucked string, minimal sustain. Bright filter lets the
        //    formant peak through.
        { "Ray Pluck", {
            {"shape1", 2}, {"shape2", 0}, {"operation", 0}, {"smoothK", 0.5f},
            {"size1", 0.5f}, {"size2", 0.3f}, {"offsetX", 0.4f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.9f}, {"scanHeight", 0.f},
            {"topoMorph", 0.05f}, {"distScale", 5.f}, {"scanMode", 1.f},
            {"attack", 0.001f}, {"decay", 0.18f}, {"sustain", 0.08f}, {"release", 0.5f},
            {"masterGain", 0.25f}, {"filterCutoff", 14000.f}, {"filterRes", 0.2f}
        }, "", 3 },

        // 4. Formant Choir — 7 interleaved rays through a twisted sphere+cylinder
        //    create multiple formant peaks like layered voices. Slow attack mimics
        //    a choir swelling in; warm 6kHz filter for vocal warmth, long sustain.
        { "Formant Choir", {
            {"shape1", 0}, {"shape2", 3}, {"operation", 0}, {"smoothK", 0.9f},
            {"size1", 0.45f}, {"size2", 0.4f}, {"offsetX", 0.25f}, {"offsetY", 0.f},
            {"twist", 1.5f}, {"scanRadius", 0.65f}, {"scanHeight", 0.f},
            {"topoMorph", 0.85f}, {"distScale", 3.f}, {"scanMode", 1.f},
            {"attack", 0.5f}, {"decay", 0.3f}, {"sustain", 0.85f}, {"release", 1.2f},
            {"masterGain", 0.2f}, {"filterCutoff", 6000.f}, {"filterRes", 0.12f}
        }, "", 4 },

        // ── ACOUSTIC TRACE (Mode 2) ─────────────────────────────────

        // 5. Bronze Bell — 64 rays bounce 3 times inside an octahedron's angular
        //    facets, creating dense inharmonic reflections. Extremely fast attack,
        //    long decay, almost no sustain — struck-bell envelope. Metal texture
        //    + Studio skybox for the bronze visual.
        { "Bronze Bell", {
            {"shape1", 4}, {"shape2", 0}, {"operation", 2}, {"smoothK", 0.25f},
            {"size1", 0.5f}, {"size2", 0.28f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.4f}, {"scanHeight", 0.f},
            {"topoMorph", 0.45f}, {"distScale", 4.f}, {"scanMode", 2.f},
            {"attack", 0.001f}, {"decay", 1.2f}, {"sustain", 0.05f}, {"release", 2.5f},
            {"masterGain", 0.22f}, {"filterCutoff", 16000.f}, {"filterRes", 0.03f}
        }, "metal", 3 },

        // 6. Metal Cavern — max 6 bounces inside a torus+box union create dense
        //    reverberant reflections like sound in a metal cave. Sustained drone,
        //    dark filter (4kHz) for that underground muffled quality, heavy resonance.
        { "Metal Cavern", {
            {"shape1", 2}, {"shape2", 1}, {"operation", 0}, {"smoothK", 0.6f},
            {"size1", 0.55f}, {"size2", 0.4f}, {"offsetX", 0.15f}, {"offsetY", 0.f},
            {"twist", 0.5f}, {"scanRadius", 0.5f}, {"scanHeight", 0.05f},
            {"topoMorph", 1.f}, {"distScale", 2.f}, {"scanMode", 2.f},
            {"attack", 0.08f}, {"decay", 0.6f}, {"sustain", 0.75f}, {"release", 2.0f},
            {"masterGain", 0.18f}, {"filterCutoff", 4000.f}, {"filterRes", 0.35f}
        }, "metal", 0 },

        // ── GRANULAR CURVATURE (Mode 3) ─────────────────────────────

        // 7. Gamelan Shimmer — 150+ grains scatter across the sharp edges of an
        //    octahedron-minus-torus. High curvature at edges → high grain frequencies
        //    → bright inharmonic shimmer like a gamelan ensemble. Quick attack with
        //    long shimmering release, wide open filter.
        { "Gamelan Shimmer", {
            {"shape1", 4}, {"shape2", 2}, {"operation", 3}, {"smoothK", 0.35f},
            {"size1", 0.45f}, {"size2", 0.3f}, {"offsetX", 0.2f}, {"offsetY", 0.f},
            {"twist", 0.f}, {"scanRadius", 0.5f}, {"scanHeight", 0.f},
            {"topoMorph", 0.9f}, {"distScale", 6.f}, {"scanMode", 3.f},
            {"attack", 0.003f}, {"decay", 0.6f}, {"sustain", 0.35f}, {"release", 2.0f},
            {"masterGain", 0.2f}, {"filterCutoff", 18000.f}, {"filterRes", 0.f}
        }, "", 2 },

        // ── VOLUMETRIC SPECTRO (Mode 4) ─────────────────────────────

        // 8. Vowel Organ — additive synthesis from 20 harmonics weighted by how close
        //    a sphere+cylinder union is to the scan cylinder at each height.
        //    The shape's profile creates natural formant peaks like vocal tract
        //    resonances. Organ-like sustain, moderate brightness for "aaah" vowel.
        { "Vowel Organ", {
            {"shape1", 0}, {"shape2", 3}, {"operation", 0}, {"smoothK", 1.0f},
            {"size1", 0.5f}, {"size2", 0.45f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.3f}, {"scanRadius", 0.48f}, {"scanHeight", 0.f},
            {"topoMorph", 0.6f}, {"distScale", 3.5f}, {"scanMode", 4.f},
            {"attack", 0.04f}, {"decay", 0.08f}, {"sustain", 0.92f}, {"release", 0.35f},
            {"masterGain", 0.22f}, {"filterCutoff", 10000.f}, {"filterRes", 0.08f}
        }, "organic", -1 },

        // ── FIELD TRAVERSE (Mode 5) ─────────────────────────────────

        // 9. Lissajous Buzz — the path traces a complex 2:3:5 Lissajous figure
        //    through a twisted torus+octahedron, crossing the surface many times
        //    per cycle → dense, aggressive buzz. Fast attack, full sustain for
        //    a lead synth, high resonance adds edge.
        { "Lissajous Buzz", {
            {"shape1", 2}, {"shape2", 4}, {"operation", 0}, {"smoothK", 0.5f},
            {"size1", 0.45f}, {"size2", 0.35f}, {"offsetX", 0.3f}, {"offsetY", 0.f},
            {"twist", 2.f}, {"scanRadius", 0.55f}, {"scanHeight", 0.f},
            {"topoMorph", 0.95f}, {"distScale", 5.f}, {"scanMode", 5.f},
            {"attack", 0.002f}, {"decay", 0.04f}, {"sustain", 0.95f}, {"release", 0.12f},
            {"masterGain", 0.18f}, {"filterCutoff", 14000.f}, {"filterRes", 0.35f}
        }, "circuit", -1 },

        // 10. Drift Pad — a near-circular path (low topoMorph) drifts gently
        //     through a soft sphere+torus blend. The path barely grazes the surface,
        //     creating subtle timbral motion. Very slow attack, endless sustain,
        //     warm 4kHz filter → ambient pad that breathes.
        { "Drift Pad", {
            {"shape1", 0}, {"shape2", 2}, {"operation", 0}, {"smoothK", 1.2f},
            {"size1", 0.5f}, {"size2", 0.4f}, {"offsetX", 0.f}, {"offsetY", 0.f},
            {"twist", 0.3f}, {"scanRadius", 0.52f}, {"scanHeight", 0.f},
            {"topoMorph", 0.1f}, {"distScale", 2.f}, {"scanMode", 5.f},
            {"attack", 1.2f}, {"decay", 0.5f}, {"sustain", 0.9f}, {"release", 3.0f},
            {"masterGain", 0.18f}, {"filterCutoff", 4000.f}, {"filterRes", 0.1f}
        }, "marble", 5 }
    };
}
