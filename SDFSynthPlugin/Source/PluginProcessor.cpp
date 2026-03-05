#include "PluginProcessor.h"
#include "PluginEditor.h"

SDFSynthProcessor::SDFSynthProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Register parameter listeners for wavetable-affecting params
    const juce::StringArray dirtyParams = {
        "shape1", "shape2", "operation", "smoothK",
        "size1", "size2", "offsetX", "offsetY", "twist",
        "scanRadius", "scanHeight", "topoMorph", "distScale",
        "dispAmt", "emIntensity", "texScale",
        "scanMode",
        "oscBScanMode", "oscBScanHeight",
        "sfM", "sfN1", "sfN2", "sfN3",
        "onionEnable", "onionThickness", "stairCount"
    };
    for (auto& p : dirtyParams)
        apvts.addParameterListener(p, this);

    // ADSR listeners
    for (auto& p : { "attack", "decay", "sustain", "release" })
        apvts.addParameterListener(juce::String(p), this);

    // Initialize LFOs
    lfo1.reset();
    lfo2.reset();

    rebuildWavetable();
}

SDFSynthProcessor::~SDFSynthProcessor()
{
    // Stop timer first to prevent new rebuild triggers
    stopTimer();

    // Signal destruction to prevent new rebuilds
    destroying.store(true, std::memory_order_relaxed);

    // Remove all listeners FIRST to prevent new callbacks during teardown
    const juce::StringArray allParams = {
        "shape1", "shape2", "operation", "smoothK",
        "size1", "size2", "offsetX", "offsetY", "twist",
        "scanRadius", "scanHeight", "topoMorph", "distScale",
        "dispAmt", "emIntensity", "texScale",
        "scanMode",
        "oscBScanMode", "oscBScanHeight",
        "attack", "decay", "sustain", "release",
        "sfM", "sfN1", "sfN2", "sfN3",
        "onionEnable", "onionThickness", "stairCount"
    };
    for (auto& p : allParams)
        apvts.removeParameterListener(p, this);

    // Now safe to join the background threads
    if (builderThread.joinable())
        builderThread.join();
    if (voxelThread.joinable())
        voxelThread.join();
}

juce::AudioProcessorValueTreeState::ParameterLayout SDFSynthProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Shape params
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("shape1", 1), "Shape A",
        juce::StringArray{ "Sphere", "Box", "Torus", "Cylinder", "Octahedron", "Custom",
                           "Capsule", "RoundBox", "HexPrism", "Torus82", "Torus88", "SuperFormula" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("shape2", 1), "Shape B",
        juce::StringArray{ "Sphere", "Box", "Torus", "Cylinder", "Octahedron", "Custom",
                           "Capsule", "RoundBox", "HexPrism", "Torus82", "Torus88", "SuperFormula" }, 2));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("operation", 1), "Operation",
        juce::StringArray{ "Smooth Union", "Union", "Intersection", "Subtraction",
                           "Smooth Intersect", "Smooth Subtract",
                           "Chamfer Union", "Chamfer Intersect", "Chamfer Subtract",
                           "Stairs Union", "Pipe" }, 0));

    // Scene params
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("smoothK", 1), "Smooth K",
        juce::NormalisableRange<float>(0.01f, 1.5f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("size1", 1), "Size A",
        juce::NormalisableRange<float>(0.1f, 0.7f, 0.01f), 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("size2", 1), "Size B",
        juce::NormalisableRange<float>(0.1f, 0.7f, 0.01f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("offsetX", 1), "Offset X",
        juce::NormalisableRange<float>(-1.f, 1.f, 0.01f), 0.45f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("offsetY", 1), "Offset Y",
        juce::NormalisableRange<float>(-1.f, 1.f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("twist", 1), "Twist",
        juce::NormalisableRange<float>(0.f, 6.f, 0.01f), 0.0f));

    // Scan params
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("scanRadius", 1), "Scan Radius",
        juce::NormalisableRange<float>(0.1f, 1.5f, 0.01f), 0.55f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("scanHeight", 1), "Scan Height",
        juce::NormalisableRange<float>(-0.9f, 0.9f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("topoMorph", 1), "MRI Morph",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("distScale", 1), "Distance Scale",
        juce::NormalisableRange<float>(0.5f, 8.f, 0.01f), 3.0f));

    // Scan mode
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("scanMode", 1), "Scan Mode",
        juce::StringArray{ "Contour", "Ray March", "Acoustic", "Granular", "Spectral", "Traverse" }, 0));

    // ADSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("attack", 1), "Attack",
        juce::NormalisableRange<float>(0.001f, 2.f, 0.001f, 0.4f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("decay", 1), "Decay",
        juce::NormalisableRange<float>(0.001f, 2.f, 0.001f, 0.4f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sustain", 1), "Sustain",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("release", 1), "Release",
        juce::NormalisableRange<float>(0.001f, 5.f, 0.001f, 0.4f), 0.3f));

    // Filter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("filterCutoff", 1), "Filter Cutoff",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("filterRes", 1), "Filter Resonance",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("filterMode", 1), "Filter Mode",
        juce::StringArray{ "Low Pass", "Band Pass", "High Pass", "Notch", "Peak" }, 0));

    // Master
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("masterGain", 1), "Master Volume",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.25f));

    // --- Phase 2 Parameters ---

    // Noise (Milestone A)
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("noiseEnable", 1), "Noise Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("noiseLevel", 1), "Noise Level",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("noiseType", 1), "Noise Type",
        juce::StringArray{ "White", "Pink", "Brown" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("noiseFilterCutoff", 1), "Noise Filter",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));

    // Oscillator Effects (Milestone B)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("oscFold", 1), "Wavefold",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("oscPhaseDist", 1), "Phase Dist",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("oscPW", 1), "Pulse Width",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("oscSync", 1), "Sync",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.0f));

    // Osc B (Milestone C)
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("oscBEnable", 1), "Osc B Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("oscBLevel", 1), "Osc B Level",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID("oscBSemitone", 1), "Osc B Semi",
        -24, 24, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("oscBFine", 1), "Osc B Fine",
        juce::NormalisableRange<float>(-100.f, 100.f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("oscBScanMode", 1), "Osc B Scan",
        juce::StringArray{ "Contour", "Ray March", "Acoustic", "Granular", "Spectral", "Traverse" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("oscBScanHeight", 1), "Osc B Height",
        juce::NormalisableRange<float>(-0.9f, 0.9f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("oscBMixMode", 1), "Osc B Mix",
        juce::StringArray{ "Add", "Ring", "FM", "AM" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("oscBFMDepth", 1), "FM Depth",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.0f));

    // Unison (Milestone D)
    params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID("unisonVoices", 1), "Unison Voices",
        1, 8, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("unisonDetune", 1), "Detune",
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f), 20.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("unisonSpread", 1), "Spread",
        juce::NormalisableRange<float>(0.f, 100.f, 0.1f), 50.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("unisonBlend", 1), "Blend",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));

    // Oversampling (Milestone E)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("oversample", 1), "Oversample",
        juce::StringArray{ "Off", "2x", "4x" }, 0));

    // Texture params
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("texScale", 1), "Texture Scale",
        juce::NormalisableRange<float>(0.3f, 6.f, 0.01f), 1.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("texBright", 1), "Texture Brightness",
        juce::NormalisableRange<float>(0.3f, 3.f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("texBlend", 1), "Texture Blend",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.75f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("normIntensity", 1), "Normal Strength",
        juce::NormalisableRange<float>(0.f, 3.f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("roughOffset", 1), "Roughness Offset",
        juce::NormalisableRange<float>(-0.5f, 0.5f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("dispAmt", 1), "Displacement",
        juce::NormalisableRange<float>(0.f, 0.4f, 0.001f), 0.08f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("aoIntensity", 1), "AO Strength",
        juce::NormalisableRange<float>(0.f, 2.f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("emIntensity", 1), "Emissive",
        juce::NormalisableRange<float>(0.f, 3.f, 0.01f), 1.0f));

    // Skybox params
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("skyboxSelect", 1), "Skybox",
        juce::StringArray{ "Void", "Dark Space", "Sunset", "Studio", "Nebula", "Blue Hour", "Custom" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("skyboxExposure", 1), "Exposure",
        juce::NormalisableRange<float>(0.f, 5.f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("skyboxRotation", 1), "Rotation",
        juce::NormalisableRange<float>(0.f, 360.f, 1.f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("skyboxReflect", 1), "Reflection",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("skyboxBlur", 1), "Blur",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));

    // --- Phase 3 Parameters ---

    // LFO 1
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("lfo1Rate", 1), "LFO 1 Rate",
        juce::NormalisableRange<float>(0.01f, 50.f, 0.01f, 0.3f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("lfo1Shape", 1), "LFO 1 Shape",
        juce::StringArray{ "Sine", "Triangle", "Saw", "Square", "S&H", "Random" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("lfo1Sync", 1), "LFO 1 Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("lfo1Phase", 1), "LFO 1 Phase",
        juce::NormalisableRange<float>(0.f, 360.f, 1.f), 0.0f));

    // LFO 2
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("lfo2Rate", 1), "LFO 2 Rate",
        juce::NormalisableRange<float>(0.01f, 50.f, 0.01f, 0.3f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("lfo2Shape", 1), "LFO 2 Shape",
        juce::StringArray{ "Sine", "Triangle", "Saw", "Square", "S&H", "Random" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("lfo2Sync", 1), "LFO 2 Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("lfo2Phase", 1), "LFO 2 Phase",
        juce::NormalisableRange<float>(0.f, 360.f, 1.f), 0.0f));

    // Macro knobs
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("macro1", 1), "Macro 1",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("macro2", 1), "Macro 2",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("macro3", 1), "Macro 3",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("macro4", 1), "Macro 4",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));

    // --- Phase 4 Parameters: Effects ---

    // Distortion
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("fxDistEnable", 1), "Dist Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxDistDrive", 1), "Dist Drive",
        juce::NormalisableRange<float>(1.f, 20.f, 0.01f, 0.5f), 1.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxDistMix", 1), "Dist Mix",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("fxDistType", 1), "Dist Type",
        juce::StringArray{ "Soft Clip", "Hard Clip", "Fold", "Bitcrush" }, 0));

    // Chorus
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("fxChorusEnable", 1), "Chorus Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxChorusRate", 1), "Chorus Rate",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.01f, 0.5f), 1.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxChorusDepth", 1), "Chorus Depth",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxChorusMix", 1), "Chorus Mix",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));

    // Delay
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("fxDelayEnable", 1), "Delay Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxDelayTime", 1), "Delay Time",
        juce::NormalisableRange<float>(10.f, 1000.f, 0.1f, 0.5f), 250.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxDelayFeedback", 1), "Delay Feedback",
        juce::NormalisableRange<float>(0.f, 0.95f, 0.01f), 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxDelayMix", 1), "Delay Mix",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));

    // Reverb
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("fxReverbEnable", 1), "Reverb Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxReverbSize", 1), "Reverb Size",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxReverbDamping", 1), "Reverb Damping",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("fxReverbMix", 1), "Reverb Mix",
        juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.3f));

    // --- SuperFormula Parameters ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sfM", 1), "SF Symmetry",
        juce::NormalisableRange<float>(1.f, 20.f, 0.1f), 6.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sfN1", 1), "SF N1",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.01f), 1.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sfN2", 1), "SF N2",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.01f), 1.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("sfN3", 1), "SF N3",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.01f), 1.f));

    // --- Onion Shell ---
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("onionEnable", 1), "Onion Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("onionThickness", 1), "Onion Thickness",
        juce::NormalisableRange<float>(0.01f, 0.2f, 0.001f), 0.05f));

    // --- Stairs Operation ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("stairCount", 1), "Stair Count",
        juce::NormalisableRange<float>(2.f, 16.f, 1.f), 4.f));

    return { params.begin(), params.end() };
}

void SDFSynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    baseSampleRate = sampleRate;
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
    filterL.reset();
    filterR.reset();

    // Noise generator
    noiseGen.reset();
    noiseGen.setSampleRate(static_cast<float>(sampleRate));
    noiseFilterL.reset();
    noiseFilterR.reset();

    // LFOs
    lfo1.reset();
    lfo2.reset();

    // Oversampling objects (2-channel, half-band polyphase IIR)
    os2x = std::make_unique<juce::dsp::Oversampling<float>>(2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    os4x = std::make_unique<juce::dsp::Oversampling<float>>(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    os2x->initProcessing(static_cast<size_t>(samplesPerBlock));
    os4x->initProcessing(static_cast<size_t>(samplesPerBlock));

    // Pre-allocate oversampling scratch buffer (max 4x)
    osScratchBuffer.setSize(2, samplesPerBlock * 4);

    // Smoothed master gain (20ms ramp)
    smoothedGain.reset(sampleRate, 0.020);
    smoothedGain.setCurrentAndTargetValue(apvts.getRawParameterValue("masterGain")->load());

    // FX chain
    fxChain.prepare(sampleRate, samplesPerBlock);

    // Reset ADAA state
    for (int ch = 0; ch < 2; ++ch)
    {
        adaaPrevX[ch] = 0.f;
        adaaPrevF[ch] = 0.f;
    }
    rebuildWavetable();

    // Start timer to poll wavetable rebuilds off the audio thread (~60Hz)
    startTimer(16);
}

void SDFSynthProcessor::releaseResources()
{
    stopTimer();
}

bool SDFSynthProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

// ADAA (Anti-Derivative Anti-Aliasing) for tanh soft clipper
// F(x) = log(cosh(x)) is the antiderivative of tanh(x)
static inline float tanhADAA(float x, float& prevX, float& prevF)
{
    // Clamp to avoid overflow in cosh()
    float cx = std::clamp(x, -10.f, 10.f);
    float F = std::log(std::cosh(cx));
    float dx = x - prevX;
    float out;
    if (std::abs(dx) > 1e-7f)
        out = (F - prevF) / dx;
    else
        out = std::tanh(cx); // fallback for near-zero dx
    prevX = x;
    prevF = F;
    // Clear denormals in state variables
    if (std::abs(prevX) < 1e-20f) prevX = 0.f;
    if (std::abs(prevF) < 1e-20f) prevF = 0.f;
    return out;
}

void SDFSynthProcessor::snapshotParams()
{
    auto load = [&](const char* id) { return apvts.getRawParameterValue(id)->load(); };

    paramCache.lfo1Rate  = load("lfo1Rate");
    paramCache.lfo1Shape = static_cast<int>(load("lfo1Shape"));
    paramCache.lfo1Sync  = load("lfo1Sync") > 0.5f;
    paramCache.lfo1Phase = load("lfo1Phase");

    paramCache.lfo2Rate  = load("lfo2Rate");
    paramCache.lfo2Shape = static_cast<int>(load("lfo2Shape"));
    paramCache.lfo2Sync  = load("lfo2Sync") > 0.5f;
    paramCache.lfo2Phase = load("lfo2Phase");

    paramCache.macro1 = load("macro1");
    paramCache.macro2 = load("macro2");
    paramCache.macro3 = load("macro3");
    paramCache.macro4 = load("macro4");

    paramCache.oversample = static_cast<int>(load("oversample"));

    paramCache.noiseEnable      = load("noiseEnable") > 0.5f;
    paramCache.noiseLevel       = load("noiseLevel");
    paramCache.noiseType        = static_cast<int>(load("noiseType"));
    paramCache.noiseFilterCutoff = load("noiseFilterCutoff");

    paramCache.oscBEnable   = load("oscBEnable") > 0.5f;
    paramCache.oscBSemitone = static_cast<int>(load("oscBSemitone"));
    paramCache.oscBFine     = load("oscBFine");
    paramCache.oscBMixMode  = static_cast<int>(load("oscBMixMode"));

    paramCache.unisonVoices = static_cast<int>(load("unisonVoices"));
    paramCache.unisonBlend  = load("unisonBlend");

    paramCache.filterMode   = static_cast<int>(load("filterMode"));
    paramCache.masterGain   = load("masterGain");

    paramCache.fxDistEnable   = load("fxDistEnable") > 0.5f;
    paramCache.fxChorusEnable = load("fxChorusEnable") > 0.5f;
    paramCache.fxDelayEnable  = load("fxDelayEnable") > 0.5f;
    paramCache.fxReverbEnable = load("fxReverbEnable") > 0.5f;

    paramCache.fxDistMix     = load("fxDistMix");
    paramCache.fxDistType    = static_cast<int>(load("fxDistType"));
    paramCache.fxChorusDepth = load("fxChorusDepth");
    paramCache.fxChorusMix   = load("fxChorusMix");
    paramCache.fxDelayTime   = load("fxDelayTime");
    paramCache.fxDelayMix    = load("fxDelayMix");
    paramCache.fxReverbDamping = load("fxReverbDamping");
    paramCache.fxReverbMix   = load("fxReverbMix");
}

void SDFSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Snapshot all APVTS params once (avoids scattered atomic loads)
    snapshotParams();

    // Merge GUI keyboard events into the MIDI stream
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // Handle MIDI CC mappings before passing to synthesiser
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isController())
            handleMidiCC(msg.getControllerNumber(), msg.getControllerValue());
        else if (msg.isChannelPressure())
            lastAftertouch.store(msg.getChannelPressureValue() / 127.f, std::memory_order_relaxed);
    }

    // Pick up new table from background thread if ready (zero-copy: shared_ptr swap only)
    if (newTableReady.exchange(false, std::memory_order_acq_rel))
    {
        currentWavetable = pendingWavetable;
        activeWavetable = pendingWavetable;
        currentContour = pendingContour;  // shared_ptr copy (8 bytes)
        currentAlgData = pendingAlgData;  // shared_ptr copy
        synthesiser.setMipMappedWavetable(pendingMipTable, CROSSFADE_SAMPLES);  // shared_ptr copy
    }

    // Pick up Osc B table
    if (newTableBReady.exchange(false, std::memory_order_acq_rel))
    {
        synthesiser.setMipMappedWavetableB(pendingMipTableB, CROSSFADE_SAMPLES);
    }

    // NOTE: Voxel pickup and wavetable rebuild triggering are handled in
    // timerCallback() to keep thread management off the real-time audio thread.

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // ================================================================
    // 1. Advance LFOs (block-rate) — uses cached params
    // ================================================================
    {
        lfo1.setRate(paramCache.lfo1Rate);
        lfo1.setShape(static_cast<LFOShape>(paramCache.lfo1Shape));
        lfo1.setSyncEnabled(paramCache.lfo1Sync);
        lfo1.setPhaseOffset(paramCache.lfo1Phase);

        double bpm = 120.0;
        if (auto* ph = getPlayHead())
        {
            auto pos = ph->getPosition();
            if (pos.hasValue() && pos->getBpm().hasValue())
                bpm = *pos->getBpm();
        }

        float l1v = lfo1.advance(numSamples, baseSampleRate, bpm);
        lfo1Value.store(l1v, std::memory_order_relaxed);

        lfo2.setRate(paramCache.lfo2Rate);
        lfo2.setShape(static_cast<LFOShape>(paramCache.lfo2Shape));
        lfo2.setSyncEnabled(paramCache.lfo2Sync);
        lfo2.setPhaseOffset(paramCache.lfo2Phase);

        float l2v = lfo2.advance(numSamples, baseSampleRate, bpm);
        lfo2Value.store(l2v, std::memory_order_relaxed);
    }

    // ================================================================
    // 2. Store all global source values into modMatrix
    // ================================================================
    float envValue = lastEnvValue.load();
    modMatrix.setSourceValue(ModSource::Envelope, envValue);
    modMatrix.setSourceValue(ModSource::LFO1, lfo1Value.load(std::memory_order_relaxed));
    modMatrix.setSourceValue(ModSource::LFO2, lfo2Value.load(std::memory_order_relaxed));
    modMatrix.setSourceValue(ModSource::ModWheel, lastModWheel.load(std::memory_order_relaxed));
    modMatrix.setSourceValue(ModSource::Aftertouch, lastAftertouch.load(std::memory_order_relaxed));
    modMatrix.setSourceValue(ModSource::Macro1, paramCache.macro1);
    modMatrix.setSourceValue(ModSource::Macro2, paramCache.macro2);
    modMatrix.setSourceValue(ModSource::Macro3, paramCache.macro3);
    modMatrix.setSourceValue(ModSource::Macro4, paramCache.macro4);

    // ================================================================
    // 3. Acquire latest routes from GUI
    // ================================================================
    modMatrix.acquire();

    // ================================================================
    // 4. Apply modulation to all modulatable params
    // ================================================================
    const auto& modIds = getModulatableParamIds();
    auto computeModVal = [&](int destIdx) -> float {
        auto* param = apvts.getParameter(modIds[static_cast<size_t>(destIdx)]);
        if (!param) return 0.f;
        float baseNorm = param->getValue();
        float modNorm = modMatrix.computeModulatedNorm(destIdx, baseNorm);
        return param->convertFrom0to1(modNorm);
    };

    // Read ADSR base values, apply modulation
    float adsrA = computeModVal(12); // attack
    float adsrD = computeModVal(13); // decay
    float adsrS = computeModVal(14); // sustain
    float adsrR = computeModVal(15); // release

    // Update ADSR
    synthesiser.updateADSR(adsrA, adsrD, adsrS, adsrR);

    // Update osc effects params with modulation
    bool oscBypassed = bypassOsc.load(std::memory_order_relaxed);

    // When osc is bypassed, skip ALL synthesis — buffer stays at zero (true mute)
    if (!oscBypassed)
    {
        float modFold = computeModVal(17); // oscFold
        float modPD   = computeModVal(18); // oscPhaseDist
        float modPW   = computeModVal(19); // oscPW
        float modSync = computeModVal(20); // oscSync
        synthesiser.updateOscEffects(modFold, modPD, modPW, modSync);

        // Update Osc B params (some modulated)
        float modOscBLevel = computeModVal(23); // oscBLevel
        float modOscBFM    = computeModVal(24); // oscBFMDepth
        synthesiser.updateOscBParams(
            paramCache.oscBEnable,
            modOscBLevel,
            paramCache.oscBSemitone,
            paramCache.oscBFine,
            paramCache.oscBMixMode,
            modOscBFM);

        // Update unison params with modulation
        float modDetune = computeModVal(21); // unisonDetune
        float modSpread = computeModVal(22); // unisonSpread
        synthesiser.updateUnisonParams(
            paramCache.unisonVoices,
            modDetune, modSpread,
            paramCache.unisonBlend);

        // === Oversampling (uses pre-allocated scratch buffer) ===
        int osFactor = (paramCache.oversample == 1) ? 2 : (paramCache.oversample == 2) ? 4 : 1;

        if (osFactor > 1 && os2x && os4x)
        {
            auto& os = (osFactor == 2) ? *os2x : *os4x;
            juce::dsp::AudioBlock<float> block(buffer);
            auto osBlock = os.processSamplesUp(block);

            int osNumSamples = static_cast<int>(osBlock.getNumSamples());
            int osNumChannels = static_cast<int>(osBlock.getNumChannels());

            // Use pre-allocated scratch buffer instead of stack allocation
            if (osScratchBuffer.getNumSamples() < osNumSamples)
                osScratchBuffer.setSize(2, osNumSamples, false, false, true);
            osScratchBuffer.clear(0, osNumSamples);

            // Scale MIDI positions by oversample factor
            juce::MidiBuffer scaledMidi;
            for (const auto metadata : midiMessages)
                scaledMidi.addEvent(metadata.getMessage(), metadata.samplePosition * osFactor);

            synthesiser.setCurrentPlaybackSampleRate(baseSampleRate * osFactor);
            synthesiser.renderNextBlock(osScratchBuffer, scaledMidi, 0, osNumSamples);

            // Copy back to oversampled block
            for (int ch = 0; ch < osNumChannels; ++ch)
                juce::FloatVectorOperations::copy(osBlock.getChannelPointer(static_cast<size_t>(ch)),
                                                   osScratchBuffer.getReadPointer(ch),
                                                   osNumSamples);

            os.processSamplesDown(block);
            synthesiser.setCurrentPlaybackSampleRate(baseSampleRate);
        }
        else
        {
            synthesiser.renderNextBlock(buffer, midiMessages, 0, numSamples);
        }
    }
    // else: oscBypassed — buffer stays cleared (true silence from oscillator)

    // Update envelope value for next frame's modulation + GUI
    envValue = synthesiser.getMaxEnvelopeValue();
    lastEnvValue.store(envValue);

    // === Noise injection (gated by envelope — silent when no voices active) ===
    if (paramCache.noiseEnable && envValue > 1e-6f && !oscBypassed)
    {
        float nLevel = computeModVal(25);  // noiseLevel (modulated)
        auto nType = static_cast<NoiseType>(paramCache.noiseType);
        float nCutoff = computeModVal(26); // noiseFilterCutoff (modulated)
        float sr = static_cast<float>(getSampleRate());

        noiseFilterL.setParams(nCutoff, 0.f, sr);
        noiseFilterR.setParams(nCutoff, 0.f, sr);
        noiseFilterL.setMode(FilterMode::LowPass);
        noiseFilterR.setMode(FilterMode::LowPass);

        for (int i = 0; i < numSamples; ++i)
        {
            float nL = noiseGen.nextSample(nType) * nLevel * envValue;
            float nR = noiseGen.nextSample(nType) * nLevel * envValue;

            if (nCutoff < 19900.f)
            {
                nL = noiseFilterL.process(nL);
                nR = noiseFilterR.process(nR);
            }

            if (numChannels >= 1) buffer.addSample(0, i, nL);
            if (numChannels >= 2) buffer.addSample(1, i, nR);
        }
    }

    bool fxBypassed = bypassFX.load(std::memory_order_relaxed);

    // SVF Filter (modulated)
    float cutoff = computeModVal(10); // filterCutoff
    float res    = computeModVal(11); // filterRes

    // Check if any scene params are modulated and need rebuild
    {
        bool sceneDirty = false;
        for (int d = 0; d < 10; ++d) // dest 0-9 are scene/scan params
        {
            if (modMatrix.hasRouteToDest(d))
            {
                sceneDirty = true;
                break;
            }
        }
        if (sceneDirty && envValue > 0.01f)
            wavetableDirty.store(true, std::memory_order_relaxed);
    }

    if (!fxBypassed)
    {
        if (cutoff != lastFilterCutoff || res != lastFilterRes)
        {
            filterL.setParams(cutoff, res, static_cast<float>(getSampleRate()));
            filterR.setParams(cutoff, res, static_cast<float>(getSampleRate()));
            lastFilterCutoff = cutoff;
            lastFilterRes = res;
        }

        auto fm = static_cast<FilterMode>(paramCache.filterMode);
        filterL.setMode(fm);
        filterR.setMode(fm);

        if (cutoff < 19900.f)
        {
            if (numChannels >= 1)
            {
                float* dataL = buffer.getWritePointer(0);
                for (int i = 0; i < numSamples; ++i)
                    dataL[i] = filterL.process(dataL[i]);
            }
            if (numChannels >= 2)
            {
                float* dataR = buffer.getWritePointer(1);
                for (int i = 0; i < numSamples; ++i)
                    dataR[i] = filterR.process(dataR[i]);
            }
        }

        // === FX Chain (Dist -> Chorus -> Delay -> Reverb) — cached enables + non-mod params ===
        {
            // Modulatable FX params
            float modDistDrive  = computeModVal(27); // fxDistDrive
            float modChorusRate = computeModVal(28); // fxChorusRate
            float modDelayFb    = computeModVal(29); // fxDelayFeedback
            float modReverbSize = computeModVal(30); // fxReverbSize

            fxChain.setDistParams(paramCache.fxDistEnable, modDistDrive, paramCache.fxDistMix,
                                  static_cast<DistortionType>(paramCache.fxDistType));
            fxChain.setChorusParams(paramCache.fxChorusEnable, modChorusRate, paramCache.fxChorusDepth, paramCache.fxChorusMix);
            fxChain.setDelayParams(paramCache.fxDelayEnable, paramCache.fxDelayTime, modDelayFb, paramCache.fxDelayMix);
            fxChain.setReverbParams(paramCache.fxReverbEnable, modReverbSize, paramCache.fxReverbDamping, paramCache.fxReverbMix);

            fxChain.process(buffer);
        }

        // ADAA tanh soft clipper
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            int ci = std::min(ch, 1);
            for (int i = 0; i < numSamples; ++i)
                data[i] = tanhADAA(data[i], adaaPrevX[ci], adaaPrevF[ci]);
        }
    }
    else
    {
        // FX bypassed: reset filter and ADAA state to prevent stale-state artifacts
        filterL.reset();
        filterR.reset();
        adaaPrevX[0] = adaaPrevX[1] = 0.f;
        adaaPrevF[0] = adaaPrevF[1] = 0.f;
    }

    // Master gain (modulated) — always applied for volume control
    {
        float gain = computeModVal(16); // masterGain
        if (smoothedGain.isSmoothing() || std::abs(gain - smoothedGain.getCurrentValue()) > 0.001f)
        {
            smoothedGain.setTargetValue(gain);
            for (int i = 0; i < numSamples; ++i)
            {
                float g = smoothedGain.getNextValue();
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.getWritePointer(ch)[i] *= g;
            }
        }
        else
        {
            buffer.applyGain(gain);
        }
    }

    // Reset ADAA state when silent to prevent stale-state pop on next note onset
    if (envValue < 1e-8f)
    {
        adaaPrevX[0] = adaaPrevX[1] = 0.f;
        adaaPrevF[0] = adaaPrevF[1] = 0.f;
    }
}

void SDFSynthProcessor::parameterChanged(const juce::String&, float)
{
    wavetableDirty.store(true, std::memory_order_relaxed);
}

void SDFSynthProcessor::timerCallback()
{
    // Pick up new voxel SDF from background import thread
    {
        auto newVoxel = voxelSwap.get();
        if (newVoxel && newVoxel != currentVoxelSDF)
        {
            currentVoxelSDF = newVoxel;
            scene.voxelSDF = currentVoxelSDF;
            wavetableDirty.store(true, std::memory_order_relaxed);
        }
    }

    // Trigger background rebuild if dirty and not already rebuilding
    if (wavetableDirty.exchange(false, std::memory_order_relaxed) && !rebuildInProgress.load(std::memory_order_relaxed))
    {
        triggerBackgroundRebuild();
    }
}

void SDFSynthProcessor::handleMidiCC(int controller, int value)
{
    float norm = value / 127.f;

    switch (controller)
    {
        case 1: // Mod wheel → stored for mod matrix
            lastModWheel.store(norm, std::memory_order_relaxed);
            break;
        case 74: // Brightness -> topoMorph (keep legacy CC mapping)
        {
            auto* param = apvts.getParameter("topoMorph");
            if (param) param->setValueNotifyingHost(norm);
            break;
        }
    }
}

void SDFSynthProcessor::updateSceneFromParams()
{
    scene.shape1 = static_cast<ShapeType>(
        static_cast<int>(apvts.getRawParameterValue("shape1")->load()));
    scene.shape2 = static_cast<ShapeType>(
        static_cast<int>(apvts.getRawParameterValue("shape2")->load()));
    scene.operation = static_cast<OperationType>(
        static_cast<int>(apvts.getRawParameterValue("operation")->load()));
    scene.smoothK = apvts.getRawParameterValue("smoothK")->load();
    scene.size1 = apvts.getRawParameterValue("size1")->load();
    scene.size2 = apvts.getRawParameterValue("size2")->load();
    scene.offsetX = apvts.getRawParameterValue("offsetX")->load();
    scene.offsetY = apvts.getRawParameterValue("offsetY")->load();
    scene.twist = apvts.getRawParameterValue("twist")->load();
    scene.sfM  = apvts.getRawParameterValue("sfM")->load();
    scene.sfN1 = apvts.getRawParameterValue("sfN1")->load();
    scene.sfN2 = apvts.getRawParameterValue("sfN2")->load();
    scene.sfN3 = apvts.getRawParameterValue("sfN3")->load();
    scene.onionEnable = apvts.getRawParameterValue("onionEnable")->load() > 0.5f;
    scene.onionThickness = apvts.getRawParameterValue("onionThickness")->load();
    scene.stairCount = apvts.getRawParameterValue("stairCount")->load();
    scene.voxelSDF = currentVoxelSDF;
}

void SDFSynthProcessor::triggerBackgroundRebuild()
{
    // Don't start new rebuilds during destruction
    if (destroying.load(std::memory_order_relaxed))
        return;

    // Join any previous thread
    if (builderThread.joinable())
        builderThread.join();

    rebuildInProgress.store(true, std::memory_order_relaxed);

    // Snapshot all params needed for rebuild (read from APVTS, safe from any thread)
    auto snapScene = scene;
    updateSceneFromParams();
    snapScene = scene;

    auto mode = static_cast<ScanMode>(
        static_cast<int>(apvts.getRawParameterValue("scanMode")->load()));
    float scanH = apvts.getRawParameterValue("scanHeight")->load();
    float scanR = apvts.getRawParameterValue("scanRadius")->load();
    float topoM = apvts.getRawParameterValue("topoMorph")->load();
    float distS = apvts.getRawParameterValue("distScale")->load();
    float dispA = apvts.getRawParameterValue("dispAmt")->load();
    float emI   = apvts.getRawParameterValue("emIntensity")->load();
    float texS  = apvts.getRawParameterValue("texScale")->load();

    // Snapshot texture data for thread safety (copy pixel vectors ~1MB)
    std::shared_ptr<TextureSlot> dSlotCopy, eSlotCopy;
    if (texSystem)
    {
        if (texSystem->dispTex.loaded)
        {
            dSlotCopy = std::make_shared<TextureSlot>();
            dSlotCopy->loaded = true;
            dSlotCopy->pixelData = texSystem->dispTex.pixelData;
            dSlotCopy->width = texSystem->dispTex.width;
            dSlotCopy->height = texSystem->dispTex.height;
        }
        if (texSystem->emitTex.loaded)
        {
            eSlotCopy = std::make_shared<TextureSlot>();
            eSlotCopy->loaded = true;
            eSlotCopy->pixelData = texSystem->emitTex.pixelData;
            eSlotCopy->width = texSystem->emitTex.width;
            eSlotCopy->height = texSystem->emitTex.height;
        }
    }

    // Apply modulation to scene snapshot values via mod matrix
    const auto& modIds = getModulatableParamIds();
    auto applyMod = [&](int destIdx) -> float {
        auto* param = apvts.getParameter(modIds[static_cast<size_t>(destIdx)]);
        if (!param) return 0.f;
        float baseNorm = param->getValue();
        float modNorm = modMatrix.computeModulatedNorm(destIdx, baseNorm);
        return param->convertFrom0to1(modNorm);
    };

    float envVal = lastEnvValue.load();
    if (envVal > 0.001f)
    {
        // Apply scene modulation
        if (modMatrix.hasRouteToDest(0))  snapScene.size1 = applyMod(0);
        if (modMatrix.hasRouteToDest(1))  snapScene.size2 = applyMod(1);
        if (modMatrix.hasRouteToDest(2))  snapScene.offsetX = applyMod(2);
        if (modMatrix.hasRouteToDest(3))  snapScene.offsetY = applyMod(3);
        if (modMatrix.hasRouteToDest(4))  snapScene.smoothK = applyMod(4);
        if (modMatrix.hasRouteToDest(5))  snapScene.twist = applyMod(5);
        if (modMatrix.hasRouteToDest(6))  scanR = applyMod(6);
        if (modMatrix.hasRouteToDest(7))  scanH = applyMod(7);
        if (modMatrix.hasRouteToDest(8))  topoM = applyMod(8);
        if (modMatrix.hasRouteToDest(9))  distS = applyMod(9);
    }

    // Snapshot Osc B params for background thread
    bool oscBEnabled = apvts.getRawParameterValue("oscBEnable")->load() > 0.5f;
    auto oscBMode = static_cast<ScanMode>(
        static_cast<int>(apvts.getRawParameterValue("oscBScanMode")->load()));
    float oscBScanH = apvts.getRawParameterValue("oscBScanHeight")->load();

    builderThread = std::thread([this, snapScene, mode, scanH, scanR, topoM, distS, dispA, emI, texS,
                                  dSlotCopy, eSlotCopy, oscBEnabled, oscBMode, oscBScanH]()
    {
        // Use local copies for computation
        SDFScene3D localScene = snapScene;
        ContourExtractor localExtractor;
        auto localContour = localExtractor.extractContour(localScene, scanH);

        auto localAlgData = std::make_shared<ScanAlgorithmData>();
        auto localWavetable = WavetableGenerator::generateWithMode(
            mode, localScene, localContour,
            scanR, scanH, topoM, distS,
            dSlotCopy.get(), dispA, eSlotCopy.get(), emI, texS,
            localAlgData.get());

        auto localMipTable = WavetableGenerator::generateMipMap(localWavetable);

        // Publish results (shared_ptr: audio thread picks up with zero copy)
        pendingWavetable = localWavetable;
        pendingContour = std::make_shared<const std::vector<ContourPoint>>(std::move(localContour));
        pendingMipTable = std::make_shared<const MipMappedWavetable>(localMipTable);
        pendingAlgData = localAlgData;
        newTableReady.store(true, std::memory_order_release);

        // Build Osc B table if enabled (same thread, sequential)
        if (oscBEnabled)
        {
            ContourExtractor localExtractorB;
            auto localContourB = localExtractorB.extractContour(localScene, oscBScanH);

            auto localWavetableB = WavetableGenerator::generateWithMode(
                oscBMode, localScene, localContourB,
                scanR, oscBScanH, topoM, distS,
                dSlotCopy.get(), dispA, eSlotCopy.get(), emI, texS);

            pendingMipTableB = std::make_shared<const MipMappedWavetable>(
                WavetableGenerator::generateMipMap(localWavetableB));
            newTableBReady.store(true, std::memory_order_release);
        }

        rebuildInProgress.store(false, std::memory_order_release);
    });
}

void SDFSynthProcessor::rebuildWavetable()
{
    updateSceneFromParams();

    auto mode = static_cast<ScanMode>(
        static_cast<int>(apvts.getRawParameterValue("scanMode")->load()));
    float scanH = apvts.getRawParameterValue("scanHeight")->load();

    // Always extract contour for visualization overlay (cheap: 512 rays)
    auto contour = contourExtractor.extractContour(scene, scanH);
    currentContour = std::make_shared<const std::vector<ContourPoint>>(std::move(contour));

    const TextureSlot* dSlot = texSystem ? &texSystem->dispTex : nullptr;
    const TextureSlot* eSlot = texSystem ? &texSystem->emitTex : nullptr;
    currentWavetable = WavetableGenerator::generateWithMode(
        mode, scene, *currentContour,
        apvts.getRawParameterValue("scanRadius")->load(),
        scanH,
        apvts.getRawParameterValue("topoMorph")->load(),
        apvts.getRawParameterValue("distScale")->load(),
        dSlot, apvts.getRawParameterValue("dispAmt")->load(),
        eSlot, apvts.getRawParameterValue("emIntensity")->load(),
        apvts.getRawParameterValue("texScale")->load());

    auto mipTable = std::make_shared<const MipMappedWavetable>(
        WavetableGenerator::generateMipMap(currentWavetable));
    synthesiser.setMipMappedWavetable(std::move(mipTable));
}

float SDFSynthProcessor::getModulatedParamValue(const juce::String& paramId) const
{
    auto* param = apvts.getParameter(paramId);
    if (!param) return apvts.getRawParameterValue(paramId)->load();

    int destIdx = getDestIndex(paramId);
    if (destIdx >= 0)
    {
        float baseNorm = param->getValue();
        float modNorm = modMatrix.computeModulatedNorm(destIdx, baseNorm);
        return param->convertFrom0to1(modNorm);
    }

    return param->convertFrom0to1(param->getValue());
}

void SDFSynthProcessor::importOBJFile(const juce::File& file)
{
    if (destroying.load(std::memory_order_relaxed))
        return;

    // Join previous voxel thread
    if (voxelThread.joinable())
        voxelThread.join();

    {
        std::lock_guard<std::mutex> lock(objPathMutex);
        customOBJPath = file.getFullPathName();
    }

    voxelizing.store(true, std::memory_order_relaxed);

    std::string path = file.getFullPathName().toStdString();
    voxelThread = std::thread([this, path]()
    {
        auto result = MeshImporter::importOBJ(path, 24);
        if (result)
            voxelSwap.set(result);
        voxelizing.store(false, std::memory_order_relaxed);
    });
}

juce::String SDFSynthProcessor::getCustomMeshName() const
{
    std::lock_guard<std::mutex> lock(objPathMutex);
    if (customOBJPath.isEmpty())
        return {};
    return juce::File(customOBJPath).getFileNameWithoutExtension();
}

std::shared_ptr<const VoxelSDF> SDFSynthProcessor::getCurrentVoxelSDF() const
{
    return currentVoxelSDF;
}

void SDFSynthProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Serialize mod matrix routes
    {
        auto& slots = modMatrix.getSlots();
        bool hasActive = false;
        for (auto& s : slots) if (s.active) { hasActive = true; break; }

        if (hasActive)
        {
            auto* matrixXml = xml->createNewChildElement("ModMatrix");
            for (int i = 0; i < MAX_MOD_SLOTS; ++i)
            {
                auto& s = slots[static_cast<size_t>(i)];
                if (!s.active) continue;
                auto* slotXml = matrixXml->createNewChildElement("Slot");
                slotXml->setAttribute("index", i);
                slotXml->setAttribute("source", static_cast<int>(s.source));
                slotXml->setAttribute("dest", static_cast<int>(s.destIndex));
                slotXml->setAttribute("depth", static_cast<double>(s.depth));
            }
        }
    }

    // Serialize custom OBJ path
    {
        std::lock_guard<std::mutex> lock(objPathMutex);
        if (customOBJPath.isNotEmpty())
            xml->setAttribute("customOBJPath", customOBJPath);
    }

    // Serialize UI state
    xml->setAttribute("windowWidth", savedWindowWidth);
    xml->setAttribute("windowHeight", savedWindowHeight);
    xml->setAttribute("uiScale", static_cast<double>(savedUiScale));

    copyXmlToBinary(*xml, destData);
}

void SDFSynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        // Deserialize mod matrix routes
        modMatrix.clearAllSlots();

        if (auto* matrixXml = xml->getChildByName("ModMatrix"))
        {
            // New format: ModMatrix with Slot elements
            for (auto* slotXml : matrixXml->getChildIterator())
            {
                if (slotXml->hasTagName("Slot"))
                {
                    int idx = slotXml->getIntAttribute("index");
                    ModSlot slot;
                    slot.source = static_cast<ModSource>(slotXml->getIntAttribute("source"));
                    slot.destIndex = static_cast<uint8_t>(slotXml->getIntAttribute("dest"));
                    slot.depth = static_cast<float>(slotXml->getDoubleAttribute("depth"));
                    slot.active = true;
                    modMatrix.setSlot(idx, slot);
                }
            }
            xml->removeChildElement(matrixXml, true);
        }
        else if (auto* routesXml = xml->getChildByName("ModRoutes"))
        {
            // Backward compatibility: old ModRoutes format → convert to matrix
            const auto& modIds = getModulatableParamIds();
            int slotIdx = 0;
            for (auto* routeXml : routesXml->getChildIterator())
            {
                if (routeXml->hasTagName("Route") && slotIdx < MAX_MOD_SLOTS)
                {
                    juce::String target = routeXml->getStringAttribute("target");
                    float depth = static_cast<float>(routeXml->getDoubleAttribute("depth"));

                    // Find dest index
                    int destIdx = -1;
                    for (int i = 0; i < static_cast<int>(modIds.size()); ++i)
                    {
                        if (modIds[static_cast<size_t>(i)] == target)
                        {
                            destIdx = i;
                            break;
                        }
                    }
                    if (destIdx >= 0)
                    {
                        ModSlot slot;
                        slot.source = ModSource::Envelope; // old system was envelope-only
                        slot.destIndex = static_cast<uint8_t>(destIdx);
                        slot.depth = depth;
                        slot.active = true;
                        modMatrix.setSlot(slotIdx++, slot);
                    }
                }
            }
            xml->removeChildElement(routesXml, true);
        }

        modMatrix.publish();

        // Restore UI state
        savedWindowWidth = xml->getIntAttribute("windowWidth", 950);
        savedWindowHeight = xml->getIntAttribute("windowHeight", 760);
        savedUiScale = static_cast<float>(xml->getDoubleAttribute("uiScale", 1.0));

        // Restore custom OBJ path and re-import if file still exists
        juce::String objPath = xml->getStringAttribute("customOBJPath");
        if (objPath.isNotEmpty())
        {
            juce::File objFile(objPath);
            if (objFile.existsAsFile())
                importOBJFile(objFile);
        }

        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        wavetableDirty.store(true, std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor* SDFSynthProcessor::createEditor()
{
    return new SDFSynthEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SDFSynthProcessor();
}
