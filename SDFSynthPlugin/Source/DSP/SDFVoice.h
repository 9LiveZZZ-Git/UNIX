#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "SDFOscillator.h"
#include "WavetableGenerator.h"
#include "Constants.h"
#include <memory>

class SDFSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
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

    void setWavetable(const float* table) { oscillator.setWavetable(table); }
    void setMipMappedWavetable(const MipMappedWavetable* mip) { oscillator.setMipMappedWavetable(mip); }
    void crossfadeToMipTable(const MipMappedWavetable* mip, int samples) { oscillator.crossfadeToMipTable(mip, samples); }
    void setADSR(float a, float d, float s, float r);
    float getPhase() const { return oscillator.getPhase(); }

    bool isInRelease() const { return !noteActive && adsr.isActive(); }
    float getVelocityGain() const { return velocityGain; }

private:
    SDFOscillator oscillator;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams{ 0.01f, 0.1f, 0.8f, 0.3f };

    float velocityGain = 0.f;
    float baseFrequency = 0.f;
    float pitchBendSemitones = 0.f;
    bool noteActive = false;

    // Voice stealing anti-click fade
    bool stealing = false;
    float stealFadeGain = 1.0f;
    float stealFadeDec = 0.0f;
};
