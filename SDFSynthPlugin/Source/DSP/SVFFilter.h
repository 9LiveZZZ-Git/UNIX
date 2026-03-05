#pragma once
#include "Constants.h"
#include <cmath>
#include <algorithm>

enum class FilterMode { LowPass, BandPass, HighPass, Notch, Peak };

// State Variable Filter (SVF) with per-sample coefficient smoothing
class SVFFilter
{
public:
    void reset()
    {
        ic1eq = 0.f;
        ic2eq = 0.f;
        smoothG = targetG = 0.f;
        smoothK = targetK = 2.f;
        smoothCoeff = 0.002f; // default, updated by setParams
    }

    void setParams(float cutoffHz, float resonance, float sampleRate)
    {
        targetG = std::tan(sdf::PI * std::min(cutoffHz, sampleRate * 0.49f) / sampleRate);
        targetK = 2.f - 2.f * std::clamp(resonance, 0.f, 0.98f);
        // Sample-rate-dependent smoothing: ~20Hz smoothing frequency
        smoothCoeff = 1.f - std::exp(-6.28318530f * 20.f / sampleRate);
    }

    void setMode(FilterMode m)
    {
        if (m != mode && !modeChanging)
        {
            prevMode = mode;
            modeChanging = true;
            modeCrossfade = 0;
        }
        mode = m;
    }

    float process(float v0)
    {
        // Per-sample coefficient smoothing (sample-rate independent ~20Hz)
        smoothG += (targetG - smoothG) * smoothCoeff;
        smoothK += (targetK - smoothK) * smoothCoeff;

        float a1 = 1.f / (1.f + smoothG * (smoothG + smoothK));
        float a2 = smoothG * a1;
        float a3 = smoothG * a2;

        float v3 = v0 - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.f * v1 - ic1eq;
        ic2eq = 2.f * v2 - ic2eq;

        float result = modeOutput(mode, v0, v1, v2);

        // Crossfade between old and new mode over ~500 samples
        if (modeChanging)
        {
            float oldResult = modeOutput(prevMode, v0, v1, v2);
            float t = static_cast<float>(modeCrossfade) / static_cast<float>(modeCrossfadeLen);
            result = oldResult + (result - oldResult) * t;
            ++modeCrossfade;
            if (modeCrossfade >= modeCrossfadeLen)
                modeChanging = false;
        }

        return result;
    }

private:
    float modeOutput(FilterMode m, float v0, float v1, float v2) const
    {
        switch (m)
        {
            case FilterMode::LowPass:  return v2;
            case FilterMode::BandPass: return v1;
            case FilterMode::HighPass: return v0 - smoothK * v1 - v2;
            case FilterMode::Notch:    return v0 - smoothK * v1;
            case FilterMode::Peak:     return v2 - (v0 - smoothK * v1 - v2);
            default:                   return v2;
        }
    }

    float ic1eq = 0.f, ic2eq = 0.f;
    float targetG = 0.f, smoothG = 0.f;
    float targetK = 2.f, smoothK = 2.f;
    float smoothCoeff = 0.002f;
    FilterMode mode = FilterMode::LowPass;

    // Mode crossfade state
    FilterMode prevMode = FilterMode::LowPass;
    bool modeChanging = false;
    int modeCrossfade = 0;
    static constexpr int modeCrossfadeLen = 500;
};
