#include "SDFVoice.h"

void SDFVoice::startNote(int midiNoteNumber, float velocity,
                          juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    baseFrequency = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
    velocityGain = velocity;
    oscillator.setFrequency(baseFrequency, static_cast<float>(getSampleRate()));
    oscillator.reset();

    adsr.setSampleRate(getSampleRate());
    adsr.setParameters(adsrParams);
    adsr.noteOn();
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
        // Voice stealing: fade out over ~5ms instead of hard cut
        stealing = true;
        stealFadeDec = 1.0f / (0.005f * static_cast<float>(getSampleRate()));
    }
}

void SDFVoice::pitchWheelMoved(int newPitchWheelValue)
{
    pitchBendSemitones = ((newPitchWheelValue - 8192) / 8192.f) * 2.f;
    oscillator.addPitchBend(pitchBendSemitones, static_cast<float>(getSampleRate()), baseFrequency);
}

void SDFVoice::controllerMoved(int, int) {}

void SDFVoice::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (!noteActive && !stealing)
        return;

    for (int i = startSample; i < startSample + numSamples; ++i)
    {
        float sample = oscillator.nextSample();
        float env = adsr.getNextSample();
        lastEnvelopeValue = env;
        float output = sample * env * velocityGain;

        if (stealing)
        {
            output *= stealFadeGain;
            stealFadeGain -= stealFadeDec;
            if (stealFadeGain <= 0.f)
            {
                stealFadeGain = 0.f;
                stealing = false;
                noteActive = false;
                adsr.reset();
                clearCurrentNote();
                return;
            }
        }

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, i, output);
    }

    if (!stealing && !adsr.isActive())
    {
        noteActive = false;
        clearCurrentNote();
    }
}

void SDFVoice::setADSR(float a, float d, float s, float r)
{
    adsrParams = { a, d, s, r };
    adsr.setParameters(adsrParams);
}
