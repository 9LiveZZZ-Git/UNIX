#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "SDFOscillator.h"
#include "OscEffects.h"
#include "WavetableGenerator.h"
#include "Constants.h"
#include <array>
#include <cmath>
#include <memory>

class SDFSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

struct UnisonSlot
{
    SDFOscillator oscA;
    SDFOscillator oscB;
    OscEffects effects;
    float detuneOffset = 0.f;   // cents
    float panL = 1.f, panR = 1.f; // constant-power pan
    float weight = 1.f;          // blend weight
};

class SDFVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SDFSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override;

    // Wavetable setters — apply to all unison slots
    void setWavetable(const float* table);
    void setMipMappedWavetable(const MipMappedWavetable* mip);
    void crossfadeToMipTable(const MipMappedWavetable* newMip, const MipMappedWavetable* oldMip, int samples);

    // Osc B wavetable setters
    void setMipMappedWavetableB(const MipMappedWavetable* mip);
    void crossfadeToMipTableB(const MipMappedWavetable* newMip, const MipMappedWavetable* oldMip, int samples);

    void setADSR(float a, float d, float s, float r);
    float getPhase() const { return unisonSlots[0].oscA.getPhase(); }

    bool isInRelease() const { return !noteActive && adsr.isActive(); }
    float getVelocityGain() const { return velocityGain; }
    float getCurrentEnvelopeValue() const { return lastEnvelopeValue; }

    // Osc effects
    void setOscEffectParams(float fold, float pd, float pw, float sync);

    // Osc B params
    void setOscBParams(bool enable, float level, int semi, float fine, int mixMode, float fmDepth);

    // Unison params
    void setUnisonParams(int voices, float detune, float spread, float blend);

private:
    void recomputeUnisonLayout();

    std::array<UnisonSlot, sdf::MAX_UNISON> unisonSlots;
    int activeUnisonCount = 1;

    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams{ 0.01f, 0.1f, 0.8f, 0.3f };

    float velocityGain = 0.f;
    float baseFrequency = 0.f;
    float pitchBendSemitones = 0.f;
    bool noteActive = false;
    float lastEnvelopeValue = 0.f;

    // Voice stealing anti-click fade
    bool stealing = false;
    float stealFadeGain = 1.0f;
    float stealFadeCoeff = 0.0f;

    // Fade-in for newly stolen voices (prevents click from old→new note transition)
    int fadeInRemaining = 0;
    int fadeInTotal = 1;

    // Osc effects cache
    float cachedFold = 0.f, cachedPD = 0.f, cachedPW = 0.5f, cachedSync = 0.f;

    // Osc B cache
    bool oscBEnabled = false;
    float oscBLevel = 0.5f;
    int oscBSemitone = 0;
    float oscBFine = 0.f;
    int oscBMixMode = 0; // 0=Add, 1=Ring, 2=FM, 3=AM
    float oscBFMDepth = 0.f;

    // Unison cache
    int unisonCount = 1;
    float unisonDetune = 20.f;
    float unisonSpread = 50.f;
    float unisonBlend = 0.5f;
};
