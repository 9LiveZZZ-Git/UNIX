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
        "scanMode"
    };
    for (auto& p : dirtyParams)
        apvts.addParameterListener(p, this);

    // ADSR listeners
    for (auto& p : { "attack", "decay", "sustain", "release" })
        apvts.addParameterListener(juce::String(p), this);

    rebuildWavetable();
}

SDFSynthProcessor::~SDFSynthProcessor()
{
    // Wait for background thread to finish before destroying
    if (builderThread.joinable())
        builderThread.join();

    const juce::StringArray allParams = {
        "shape1", "shape2", "operation", "smoothK",
        "size1", "size2", "offsetX", "offsetY", "twist",
        "scanRadius", "scanHeight", "topoMorph", "distScale",
        "dispAmt", "emIntensity", "texScale",
        "scanMode",
        "attack", "decay", "sustain", "release"
    };
    for (auto& p : allParams)
        apvts.removeParameterListener(p, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout SDFSynthProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Shape params
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("shape1", 1), "Shape A",
        juce::StringArray{ "Sphere", "Box", "Torus", "Cylinder", "Octahedron", "Custom" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("shape2", 1), "Shape B",
        juce::StringArray{ "Sphere", "Box", "Torus", "Cylinder", "Octahedron", "Custom" }, 2));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("operation", 1), "Operation",
        juce::StringArray{ "Smooth Union", "Union", "Intersection", "Subtraction" }, 0));

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

    return { params.begin(), params.end() };
}

void SDFSynthProcessor::prepareToPlay(double sampleRate, int)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
    filterL.reset();
    filterR.reset();
    // Reset ADAA state
    for (int ch = 0; ch < 2; ++ch)
    {
        adaaPrevX[ch] = 0.f;
        adaaPrevF[ch] = 0.f;
    }
    rebuildWavetable();
}

void SDFSynthProcessor::releaseResources() {}

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
    if (std::abs(dx) > 1e-5f)
        out = (F - prevF) / dx;
    else
        out = std::tanh(cx); // fallback for near-zero dx
    prevX = x;
    prevF = F;
    return out;
}

void SDFSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Merge GUI keyboard events into the MIDI stream
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // Handle MIDI CC mappings before passing to synthesiser
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isController())
            handleMidiCC(msg.getControllerNumber(), msg.getControllerValue());
        else if (msg.isChannelPressure())
        {
            float norm = msg.getChannelPressureValue() / 127.f;
            auto* param = apvts.getParameter("distScale");
            if (param) param->setValueNotifyingHost(param->convertTo0to1(0.5f + norm * 7.5f));
        }
    }

    // Pick up new table from background thread if ready
    if (newTableReady.exchange(false))
    {
        currentWavetable = pendingWavetable;
        activeWavetable = pendingWavetable;
        currentContour = pendingContour;

        // Crossfade happens at oscillator level (per-voice, per-sample)
        synthesiser.setMipMappedWavetable(pendingMipTable, CROSSFADE_SAMPLES);
    }

    // Trigger background rebuild if dirty and not already rebuilding
    if (wavetableDirty.exchange(false) && !rebuildInProgress.load())
    {
        triggerBackgroundRebuild();
    }

    // Read ADSR base values
    float adsrA = apvts.getRawParameterValue("attack")->load();
    float adsrD = apvts.getRawParameterValue("decay")->load();
    float adsrS = apvts.getRawParameterValue("sustain")->load();
    float adsrR = apvts.getRawParameterValue("release")->load();

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Use previous frame's envelope value for this block's modulation
    float envValue = lastEnvValue.load();

    // Apply modulation to ADSR before updating synthesiser
    {
        std::lock_guard<std::mutex> lock(modRouteMutex);
        for (const auto& route : modRoutes)
        {
            auto* param = apvts.getParameter(route.targetParamId);
            if (!param) continue;

            // Only modulate ADSR params pre-render
            if (route.targetParamId == "attack" ||
                route.targetParamId == "decay" ||
                route.targetParamId == "sustain" ||
                route.targetParamId == "release")
            {
                float baseNorm = param->getValue();
                float modNorm = std::clamp(baseNorm + envValue * route.depth, 0.f, 1.f);
                float modVal = param->convertFrom0to1(modNorm);

                if (route.targetParamId == "attack")  adsrA = modVal;
                else if (route.targetParamId == "decay")   adsrD = modVal;
                else if (route.targetParamId == "sustain") adsrS = modVal;
                else if (route.targetParamId == "release") adsrR = modVal;
            }
        }
    }

    // Update ADSR (with modulated values)
    synthesiser.updateADSR(adsrA, adsrD, adsrS, adsrR);

    synthesiser.renderNextBlock(buffer, midiMessages, 0, numSamples);

    // Update envelope value for next frame's modulation + GUI
    envValue = synthesiser.getMaxEnvelopeValue();
    lastEnvValue.store(envValue);

    // SVF Filter — update target params (smoothing happens per-sample inside process())
    float cutoff = apvts.getRawParameterValue("filterCutoff")->load();
    float res = apvts.getRawParameterValue("filterRes")->load();
    float gain = apvts.getRawParameterValue("masterGain")->load();

    // Apply modulation routes (post-render params: filter, gain, scene/scan)
    {
        bool sceneDirty = false;
        std::lock_guard<std::mutex> lock(modRouteMutex);
        for (const auto& route : modRoutes)
        {
            // Skip ADSR — already handled above
            if (route.targetParamId == "attack" || route.targetParamId == "decay" ||
                route.targetParamId == "sustain" || route.targetParamId == "release")
                continue;

            auto* param = apvts.getParameter(route.targetParamId);
            if (!param) continue;
            float baseNorm = param->getValue();
            float modNorm = std::clamp(baseNorm + envValue * route.depth, 0.f, 1.f);
            float modVal = param->convertFrom0to1(modNorm);

            if (route.targetParamId == "filterCutoff")
                cutoff = modVal;
            else if (route.targetParamId == "filterRes")
                res = modVal;
            else if (route.targetParamId == "masterGain")
                gain = modVal;
            else
                sceneDirty = true; // scene/scan param modulated — mark dirty
        }

        // Scene/scan modulation triggers wavetable rebuild (rate-limited by rebuild thread)
        if (sceneDirty && envValue > 0.01f)
            wavetableDirty.store(true);
    }

    if (cutoff != lastFilterCutoff || res != lastFilterRes)
    {
        filterL.setParams(cutoff, res, static_cast<float>(getSampleRate()));
        filterR.setParams(cutoff, res, static_cast<float>(getSampleRate()));
        lastFilterCutoff = cutoff;
        lastFilterRes = res;
    }

    // Set filter mode
    auto fmIdx = static_cast<int>(apvts.getRawParameterValue("filterMode")->load());
    auto fm = static_cast<FilterMode>(fmIdx);
    filterL.setMode(fm);
    filterR.setMode(fm);

    if (cutoff < 19900.f) // Only filter when cutoff is below ~20kHz
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

    // Apply master gain
    buffer.applyGain(gain);

    // ADAA tanh soft clipper (anti-aliased)
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        int ci = std::min(ch, 1); // index into adaa state arrays
        for (int i = 0; i < numSamples; ++i)
            data[i] = tanhADAA(data[i], adaaPrevX[ci], adaaPrevF[ci]);
    }
}

void SDFSynthProcessor::parameterChanged(const juce::String&, float)
{
    wavetableDirty.store(true);
}

void SDFSynthProcessor::handleMidiCC(int controller, int value)
{
    float norm = value / 127.f;

    switch (controller)
    {
        case 1: // Mod wheel -> scanHeight (-0.9 to 0.9)
        {
            auto* param = apvts.getParameter("scanHeight");
            if (param) param->setValueNotifyingHost(norm);
            break;
        }
        case 74: // Brightness -> topoMorph
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
}

void SDFSynthProcessor::triggerBackgroundRebuild()
{
    // Join any previous thread
    if (builderThread.joinable())
        builderThread.join();

    rebuildInProgress.store(true);

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

    // Capture texture pointers (safe to read from background thread since pixel data is stable once loaded)
    const TextureSlot* dSlot = texSystem ? &texSystem->dispTex : nullptr;
    const TextureSlot* eSlot = texSystem ? &texSystem->emitTex : nullptr;

    // Apply envelope modulation to scene/scan snapshot values
    float envValue = lastEnvValue.load();
    if (envValue > 0.001f)
    {
        std::lock_guard<std::mutex> lock(modRouteMutex);
        for (const auto& route : modRoutes)
        {
            auto* param = apvts.getParameter(route.targetParamId);
            if (!param) continue;
            float baseNorm = param->getValue();
            float modNorm = std::clamp(baseNorm + envValue * route.depth, 0.f, 1.f);
            float modVal = param->convertFrom0to1(modNorm);

            if (route.targetParamId == "size1")        snapScene.size1 = modVal;
            else if (route.targetParamId == "size2")   snapScene.size2 = modVal;
            else if (route.targetParamId == "offsetX") snapScene.offsetX = modVal;
            else if (route.targetParamId == "offsetY") snapScene.offsetY = modVal;
            else if (route.targetParamId == "smoothK") snapScene.smoothK = modVal;
            else if (route.targetParamId == "twist")   snapScene.twist = modVal;
            else if (route.targetParamId == "scanRadius") scanR = modVal;
            else if (route.targetParamId == "scanHeight") scanH = modVal;
            else if (route.targetParamId == "topoMorph")  topoM = modVal;
            else if (route.targetParamId == "distScale")  distS = modVal;
        }
    }

    builderThread = std::thread([this, snapScene, mode, scanH, scanR, topoM, distS, dispA, emI, texS, dSlot, eSlot]()
    {
        // Use local copies for computation
        SDFScene3D localScene = snapScene;
        ContourExtractor localExtractor;
        std::vector<ContourPoint> localContour;

        if (mode == ScanMode::Contour)
            localContour = localExtractor.extractContour(localScene, scanH);

        auto localWavetable = WavetableGenerator::generateWithMode(
            mode, localScene, localContour,
            scanR, scanH, topoM, distS,
            dSlot, dispA, eSlot, emI, texS);

        auto localMipTable = WavetableGenerator::generateMipMap(localWavetable);

        // Publish results
        pendingWavetable = localWavetable;
        pendingContour = std::move(localContour);
        pendingMipTable = localMipTable;

        rebuildInProgress.store(false);
        newTableReady.store(true);
    });
}

void SDFSynthProcessor::rebuildWavetable()
{
    updateSceneFromParams();

    auto mode = static_cast<ScanMode>(
        static_cast<int>(apvts.getRawParameterValue("scanMode")->load()));
    float scanH = apvts.getRawParameterValue("scanHeight")->load();

    if (mode == ScanMode::Contour)
        currentContour = contourExtractor.extractContour(scene, scanH);

    const TextureSlot* dSlot = texSystem ? &texSystem->dispTex : nullptr;
    const TextureSlot* eSlot = texSystem ? &texSystem->emitTex : nullptr;
    currentWavetable = WavetableGenerator::generateWithMode(
        mode, scene, currentContour,
        apvts.getRawParameterValue("scanRadius")->load(),
        scanH,
        apvts.getRawParameterValue("topoMorph")->load(),
        apvts.getRawParameterValue("distScale")->load(),
        dSlot, apvts.getRawParameterValue("dispAmt")->load(),
        eSlot, apvts.getRawParameterValue("emIntensity")->load(),
        apvts.getRawParameterValue("texScale")->load());

    auto mipTable = WavetableGenerator::generateMipMap(currentWavetable);
    synthesiser.setMipMappedWavetable(mipTable);
}

void SDFSynthProcessor::addModRoute(const juce::String& targetParamId, float depth)
{
    std::lock_guard<std::mutex> lock(modRouteMutex);
    for (auto& r : modRoutes)
    {
        if (r.targetParamId == targetParamId)
        {
            r.depth = depth;
            return;
        }
    }
    modRoutes.push_back({ targetParamId, depth });
}

void SDFSynthProcessor::removeModRoute(const juce::String& targetParamId)
{
    std::lock_guard<std::mutex> lock(modRouteMutex);
    modRoutes.erase(
        std::remove_if(modRoutes.begin(), modRoutes.end(),
            [&](const ModRoute& r) { return r.targetParamId == targetParamId; }),
        modRoutes.end());
}

void SDFSynthProcessor::setModDepth(const juce::String& targetParamId, float depth)
{
    std::lock_guard<std::mutex> lock(modRouteMutex);
    for (auto& r : modRoutes)
    {
        if (r.targetParamId == targetParamId)
        {
            r.depth = depth;
            return;
        }
    }
}

std::vector<ModRoute> SDFSynthProcessor::getModRoutes() const
{
    std::lock_guard<std::mutex> lock(modRouteMutex);
    return modRoutes;
}

float SDFSynthProcessor::getModulatedParamValue(const juce::String& paramId) const
{
    auto* param = apvts.getParameter(paramId);
    if (!param) return apvts.getRawParameterValue(paramId)->load();

    float baseNorm = param->getValue();
    float envValue = lastEnvValue.load();

    if (envValue > 0.001f)
    {
        std::lock_guard<std::mutex> lock(modRouteMutex);
        for (const auto& route : modRoutes)
        {
            if (route.targetParamId == paramId)
            {
                float modNorm = std::clamp(baseNorm + envValue * route.depth, 0.f, 1.f);
                return param->convertFrom0to1(modNorm);
            }
        }
    }

    return param->convertFrom0to1(baseNorm);
}

void SDFSynthProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Serialize mod routes
    {
        std::lock_guard<std::mutex> lock(modRouteMutex);
        if (!modRoutes.empty())
        {
            auto* routesXml = xml->createNewChildElement("ModRoutes");
            for (const auto& r : modRoutes)
            {
                auto* routeXml = routesXml->createNewChildElement("Route");
                routeXml->setAttribute("target", r.targetParamId);
                routeXml->setAttribute("depth", static_cast<double>(r.depth));
            }
        }
    }

    copyXmlToBinary(*xml, destData);
}

void SDFSynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        // Deserialize mod routes before replacing state
        {
            std::lock_guard<std::mutex> lock(modRouteMutex);
            modRoutes.clear();
            if (auto* routesXml = xml->getChildByName("ModRoutes"))
            {
                for (auto* routeXml : routesXml->getChildIterator())
                {
                    if (routeXml->hasTagName("Route"))
                    {
                        ModRoute r;
                        r.targetParamId = routeXml->getStringAttribute("target");
                        r.depth = static_cast<float>(routeXml->getDoubleAttribute("depth"));
                        modRoutes.push_back(r);
                    }
                }
                xml->removeChildElement(routesXml, true);
            }
        }

        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        wavetableDirty.store(true);
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
