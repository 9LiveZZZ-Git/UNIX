#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <algorithm>

enum class DistortionType { SoftClip = 0, HardClip, Fold, Bitcrush };

class FXChain
{
public:
    void prepare(double sr, int blockSize)
    {
        sampleRate = sr;

        // Chorus
        juce::dsp::ProcessSpec spec{ sr, static_cast<juce::uint32>(blockSize), 2 };
        chorus.prepare(spec);
        chorus.setCentreDelay(7.f);
        chorus.setFeedback(0.3f);

        // Delay lines (stereo ping-pong, 2 sec max)
        int maxDelaySamples = static_cast<int>(sr * 2.0);
        delayL.reset();
        delayR.reset();
        delayL.prepare(spec);
        delayR.prepare(spec);
        delayL.setMaximumDelayInSamples(maxDelaySamples);
        delayR.setMaximumDelayInSamples(maxDelaySamples);

        // Reverb
        reverb.reset();
        reverb.setSampleRate(sr);
    }

    void reset()
    {
        chorus.reset();
        delayL.reset();
        delayR.reset();
        reverb.reset();
    }

    void setDistParams(bool enable, float drive, float mix, DistortionType type)
    {
        distEnabled = enable;
        distDrive = drive;
        distMix = mix;
        distType = type;
    }

    void setChorusParams(bool enable, float rate, float depth, float mix)
    {
        chorusEnabled = enable;
        chorus.setRate(rate);
        chorus.setDepth(depth);
        chorusMix = mix;
    }

    void setDelayParams(bool enable, float timeMs, float feedback, float mix)
    {
        delayEnabled = enable;
        delayTimeMs = timeMs;
        delayFeedback = std::clamp(feedback, 0.f, 0.95f);
        delayMix = mix;
    }

    void setReverbParams(bool enable, float size, float damping, float mix)
    {
        reverbEnabled = enable;
        juce::Reverb::Parameters rp;
        rp.roomSize = size;
        rp.damping = damping;
        rp.wetLevel = 1.f;
        rp.dryLevel = 0.f;
        rp.width = 1.f;
        rp.freezeMode = 0.f;
        reverb.setParameters(rp);
        reverbMix = mix;
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        int numSamples = buffer.getNumSamples();
        int numChannels = buffer.getNumChannels();

        // --- Distortion ---
        if (distEnabled)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    float dry = data[i];
                    float wet = applyDistortion(dry);
                    data[i] = dry + distMix * (wet - dry);
                }
            }
        }

        // --- Chorus ---
        if (chorusEnabled)
        {
            // Store dry signal
            juce::AudioBuffer<float> dry(numChannels, numSamples);
            for (int ch = 0; ch < numChannels; ++ch)
                dry.copyFrom(ch, 0, buffer, ch, 0, numSamples);

            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> ctx(block);
            chorus.process(ctx);

            // Mix
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* out = buffer.getWritePointer(ch);
                const float* dryData = dry.getReadPointer(ch);
                for (int i = 0; i < numSamples; ++i)
                    out[i] = dryData[i] + chorusMix * (out[i] - dryData[i]);
            }
        }

        // --- Delay (stereo ping-pong) ---
        if (delayEnabled && numChannels >= 2)
        {
            float delaySamples = static_cast<float>(delayTimeMs * 0.001 * sampleRate);
            float* dataL = buffer.getWritePointer(0);
            float* dataR = buffer.getWritePointer(1);

            for (int i = 0; i < numSamples; ++i)
            {
                float dryL = dataL[i];
                float dryR = dataR[i];

                float tapL = delayL.popSample(0, delaySamples);
                float tapR = delayR.popSample(0, delaySamples);

                // Ping-pong: L feeds into R delay, R feeds into L delay
                delayL.pushSample(0, dryL + tapR * delayFeedback);
                delayR.pushSample(0, dryR + tapL * delayFeedback);

                dataL[i] = dryL + delayMix * tapL;
                dataR[i] = dryR + delayMix * tapR;
            }
        }
        else if (delayEnabled && numChannels == 1)
        {
            float delaySamples = static_cast<float>(delayTimeMs * 0.001 * sampleRate);
            float* data = buffer.getWritePointer(0);

            for (int i = 0; i < numSamples; ++i)
            {
                float dry = data[i];
                float tap = delayL.popSample(0, delaySamples);
                delayL.pushSample(0, dry + tap * delayFeedback);
                data[i] = dry + delayMix * tap;
            }
        }

        // --- Reverb ---
        if (reverbEnabled)
        {
            // Store dry signal
            juce::AudioBuffer<float> dry(numChannels, numSamples);
            for (int ch = 0; ch < numChannels; ++ch)
                dry.copyFrom(ch, 0, buffer, ch, 0, numSamples);

            if (numChannels >= 2)
                reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
            else
                reverb.processMono(buffer.getWritePointer(0), numSamples);

            // Mix
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* out = buffer.getWritePointer(ch);
                const float* dryData = dry.getReadPointer(ch);
                for (int i = 0; i < numSamples; ++i)
                    out[i] = dryData[i] + reverbMix * (out[i] - dryData[i]);
            }
        }
    }

private:
    float applyDistortion(float x) const
    {
        switch (distType)
        {
            case DistortionType::SoftClip:
                return std::tanh(x * distDrive);
            case DistortionType::HardClip:
                return std::clamp(x * distDrive, -1.f, 1.f);
            case DistortionType::Fold:
                return std::sin(x * distDrive * 3.14159265f);
            case DistortionType::Bitcrush:
            {
                float q = std::max(2.f, std::round(distDrive * 2.f));
                return std::round(x * q) / q;
            }
            default:
                return x;
        }
    }

    double sampleRate = 44100.0;

    // Distortion
    bool distEnabled = false;
    float distDrive = 1.f;
    float distMix = 0.5f;
    DistortionType distType = DistortionType::SoftClip;

    // Chorus
    bool chorusEnabled = false;
    juce::dsp::Chorus<float> chorus;
    float chorusMix = 0.3f;

    // Delay
    bool delayEnabled = false;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayL{ 88200 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayR{ 88200 };
    float delayTimeMs = 250.f;
    float delayFeedback = 0.4f;
    float delayMix = 0.3f;

    // Reverb
    bool reverbEnabled = false;
    juce::Reverb reverb;
    float reverbMix = 0.3f;
};
