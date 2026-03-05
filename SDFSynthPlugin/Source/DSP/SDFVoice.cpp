#include "SDFVoice.h"

void SDFVoice::setWavetable(const float* table)
{
    for (auto& slot : unisonSlots)
        slot.oscA.setWavetable(table);
}

void SDFVoice::setMipMappedWavetable(const MipMappedWavetable* mip)
{
    for (auto& slot : unisonSlots)
        slot.oscA.setMipMappedWavetable(mip);
}

void SDFVoice::crossfadeToMipTable(const MipMappedWavetable* newMip, const MipMappedWavetable* oldMip, int samples)
{
    for (auto& slot : unisonSlots)
        slot.oscA.crossfadeToMipTable(newMip, oldMip, samples);
}

void SDFVoice::setMipMappedWavetableB(const MipMappedWavetable* mip)
{
    for (auto& slot : unisonSlots)
        slot.oscB.setMipMappedWavetable(mip);
}

void SDFVoice::crossfadeToMipTableB(const MipMappedWavetable* newMip, const MipMappedWavetable* oldMip, int samples)
{
    for (auto& slot : unisonSlots)
        slot.oscB.crossfadeToMipTable(newMip, oldMip, samples);
}

void SDFVoice::setADSR(float a, float d, float s, float r)
{
    adsrParams = { a, d, s, r };
    adsr.setParameters(adsrParams);
}

void SDFVoice::setOscEffectParams(float fold, float pd, float pw, float sync)
{
    cachedFold = fold;
    cachedPD = pd;
    cachedPW = pw;
    cachedSync = sync;
    for (int i = 0; i < activeUnisonCount; ++i)
        unisonSlots[i].effects.setParams(fold, pd, pw, sync);
}

void SDFVoice::setOscBParams(bool enable, float level, int semi, float fine, int mixMode, float fmDepth)
{
    oscBEnabled = enable;
    oscBLevel = level;
    oscBSemitone = semi;
    oscBFine = fine;
    oscBMixMode = mixMode;
    oscBFMDepth = fmDepth;
}

void SDFVoice::setUnisonParams(int voices, float detune, float spread, float blend)
{
    bool changed = (voices != unisonCount) || (detune != unisonDetune) ||
                   (spread != unisonSpread) || (blend != unisonBlend);
    unisonCount = voices;
    unisonDetune = detune;
    unisonSpread = spread;
    unisonBlend = blend;
    if (changed)
        recomputeUnisonLayout();
}

void SDFVoice::recomputeUnisonLayout()
{
    activeUnisonCount = std::max(1, std::min(unisonCount, sdf::MAX_UNISON));
    int N = activeUnisonCount;

    float totalWeight = 0.f;
    for (int i = 0; i < N; ++i)
    {
        auto& slot = unisonSlots[i];

        if (N == 1)
        {
            slot.detuneOffset = 0.f;
            slot.panL = 0.7071f;
            slot.panR = 0.7071f;
            slot.weight = 1.f;
        }
        else
        {
            // Symmetric detune spread
            float t = static_cast<float>(i) / static_cast<float>(N - 1); // 0..1
            slot.detuneOffset = unisonDetune * (2.f * t - 1.f);

            // Constant-power panning
            float pan = (unisonSpread / 100.f) * (2.f * t - 1.f); // -1..+1
            float panAngle = (pan * 0.5f + 0.5f) * sdf::PI * 0.5f; // 0..PI/2
            slot.panL = std::cos(panAngle);
            slot.panR = std::sin(panAngle);

            // Blend weight: center voices louder when blend < 1
            float maxOffset = unisonDetune;
            if (maxOffset > 0.001f)
                slot.weight = unisonBlend + (1.f - unisonBlend) * (1.f - std::abs(slot.detuneOffset) / maxOffset);
            else
                slot.weight = 1.f;
        }
        totalWeight += slot.weight;
    }

    // Normalize weights
    if (totalWeight > 0.001f)
    {
        for (int i = 0; i < N; ++i)
            unisonSlots[i].weight /= totalWeight;
    }

    // Update slot frequencies if note is active
    if (noteActive || stealing)
    {
        float sr = static_cast<float>(getSampleRate());
        for (int i = 0; i < N; ++i)
        {
            float ratio = std::pow(2.f, (pitchBendSemitones + unisonSlots[i].detuneOffset / 100.f) / 12.f);
            unisonSlots[i].oscA.setFrequency(baseFrequency * ratio, sr);

            if (oscBEnabled)
            {
                float bRatio = std::pow(2.f, (pitchBendSemitones + oscBSemitone + oscBFine / 100.f + unisonSlots[i].detuneOffset / 100.f) / 12.f);
                unisonSlots[i].oscB.setFrequency(baseFrequency * bRatio, sr);
            }
        }
    }
}

void SDFVoice::startNote(int midiNoteNumber, float velocity,
                          juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    baseFrequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
    velocityGain = velocity;
    float sr = static_cast<float>(getSampleRate());

    recomputeUnisonLayout();

    for (int i = 0; i < activeUnisonCount; ++i)
    {
        auto& slot = unisonSlots[i];
        float ratio = std::pow(2.f, slot.detuneOffset / 100.f / 12.f);
        slot.oscA.setFrequency(baseFrequency * ratio, sr);
        slot.oscA.reset();
        slot.effects.reset();

        if (oscBEnabled)
        {
            float bRatio = std::pow(2.f, (oscBSemitone + oscBFine / 100.f + slot.detuneOffset / 100.f) / 12.f);
            slot.oscB.setFrequency(baseFrequency * bRatio, sr);
            slot.oscB.reset();
        }
    }

    adsr.setSampleRate(getSampleRate());
    adsr.setParameters(adsrParams);
    adsr.noteOn();

    // If this voice was active (stolen), apply a 5ms fade-in to prevent click
    if (noteActive || stealing)
    {
        fadeInTotal = std::max(1, static_cast<int>(0.005f * static_cast<float>(getSampleRate())));
        fadeInRemaining = fadeInTotal;
    }
    else
    {
        fadeInRemaining = 0;
    }

    noteActive = true;
    stealing = false;
    stealFadeGain = 1.0f;

    pitchWheelMoved(currentPitchWheelPosition);
}

void SDFVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        stealing = true;
        stealFadeCoeff = std::pow(0.001f, 1.0f / (0.005f * static_cast<float>(getSampleRate())));
    }
}

void SDFVoice::pitchWheelMoved(int newPitchWheelValue)
{
    pitchBendSemitones = ((newPitchWheelValue - 8192) / 8192.f) * 2.f;
    float sr = static_cast<float>(getSampleRate());

    for (int i = 0; i < activeUnisonCount; ++i)
    {
        auto& slot = unisonSlots[i];
        float ratio = std::pow(2.f, (pitchBendSemitones + slot.detuneOffset / 100.f) / 12.f);
        slot.oscA.setFrequency(baseFrequency * ratio, sr);

        if (oscBEnabled)
        {
            float bRatio = std::pow(2.f, (pitchBendSemitones + oscBSemitone + oscBFine / 100.f + slot.detuneOffset / 100.f) / 12.f);
            slot.oscB.setFrequency(baseFrequency * bRatio, sr);
        }
    }
}

void SDFVoice::controllerMoved(int, int) {}

void SDFVoice::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (!noteActive && !stealing)
        return;

    int numChannels = buffer.getNumChannels();

    for (int i = startSample; i < startSample + numSamples; ++i)
    {
        float env = adsr.getNextSample();
        lastEnvelopeValue = env;

        float sumL = 0.f, sumR = 0.f;

        for (int u = 0; u < activeUnisonCount; ++u)
        {
            auto& slot = unisonSlots[u];
            float phaseOffset = slot.effects.getPhaseOffset(slot.oscA.getPhase());

            float sampleA;
            if (oscBEnabled && oscBMixMode == 2 && oscBFMDepth > 0.001f)
            {
                // FM: Osc B modulates Osc A phase
                float sampleB = slot.oscB.nextSample();
                float fmOffset = sampleB * oscBFMDepth;
                sampleA = slot.oscA.nextSample(phaseOffset + fmOffset);
            }
            else
            {
                sampleA = (std::abs(phaseOffset) > 0.0001f)
                    ? slot.oscA.nextSample(phaseOffset)
                    : slot.oscA.nextSample();
            }

            // Apply osc effects (fold, PW, sync)
            sampleA = slot.effects.process(sampleA, slot.oscA.getPhase(), slot.oscA.getPhaseInc());

            float output = sampleA;

            // Mix Osc B (non-FM modes)
            if (oscBEnabled && oscBMixMode != 2)
            {
                float sampleB = slot.oscB.nextSample();
                switch (oscBMixMode)
                {
                    case 0: // Add
                        output = sampleA + sampleB * oscBLevel;
                        break;
                    case 1: // Ring
                        output = sampleA * sampleB * oscBLevel;
                        break;
                    case 3: // AM
                        output = sampleA * (1.f + sampleB * oscBLevel);
                        break;
                    default:
                        break;
                }
            }

            sumL += output * slot.weight * slot.panL;
            sumR += output * slot.weight * slot.panR;
        }

        float gainMul = env * velocityGain;

        // Fade-in after voice steal (prevents click from old→new note transition)
        if (fadeInRemaining > 0)
        {
            float t = 1.0f - static_cast<float>(fadeInRemaining) / static_cast<float>(fadeInTotal);
            gainMul *= t * t; // quadratic ease-in for smoother onset
            --fadeInRemaining;
        }

        if (stealing)
        {
            gainMul *= stealFadeGain;
            stealFadeGain *= stealFadeCoeff;
            if (stealFadeGain <= 0.001f)
            {
                stealFadeGain = 0.f;
                stealing = false;
                noteActive = false;
                adsr.reset();
                clearCurrentNote();
                return;
            }
        }

        sumL *= gainMul;
        sumR *= gainMul;

        if (numChannels >= 1) buffer.addSample(0, i, sumL);
        if (numChannels >= 2) buffer.addSample(1, i, sumR);
    }

    if (!stealing && !adsr.isActive())
    {
        noteActive = false;
        clearCurrentNote();
    }
}
