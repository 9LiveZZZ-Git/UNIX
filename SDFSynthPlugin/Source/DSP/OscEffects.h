#pragma once
#include "Constants.h"
#include <cmath>
#include <algorithm>

class OscEffects
{
public:
    void setParams(float fold, float pd, float pw, float sync)
    {
        foldAmt = fold;
        pdAmt = pd;
        pwAmt = pw;
        syncAmt = sync;
    }

    void reset()
    {
        syncPhase = 0.f;
        adaaPrevX = 0.f;
        adaaPrevF = 0.f;
    }

    // Returns phase distortion offset to apply before wavetable lookup
    float getPhaseOffset(float phase) const
    {
        if (pdAmt < 0.001f) return 0.f;
        return pdAmt * 0.5f * std::sin(sdf::TWO_PI * phase);
    }

    // Process sample after wavetable lookup (wavefolding, PW, sync mix)
    float process(float sample, float phase, float phaseInc)
    {
        // Pulse width modulation
        if (std::abs(pwAmt - 0.5f) > 0.01f)
        {
            float pwSig = (phase < pwAmt) ? 1.f : -1.f;
            float blend = std::abs(pwAmt - 0.5f) * 2.f;
            sample = sample * (1.f - blend) + pwSig * blend;
        }

        // Wavefolding with first-order ADAA
        if (foldAmt > 0.001f)
        {
            float a = (1.f + foldAmt * 7.f) * sdf::PI;
            float x = sample;
            // Antiderivative: F(x) = -cos(a*x) / a
            float F = -std::cos(a * x) / a;
            float dx = x - adaaPrevX;
            float folded;
            if (std::abs(dx) > 1e-7f)
                folded = (F - adaaPrevF) / dx;
            else
                folded = std::sin(a * x);
            adaaPrevX = x;
            adaaPrevF = F;
            sample = folded;
        }

        // Hard sync: sub-oscillator resets on master phase wrap
        if (syncAmt > 0.001f)
        {
            float syncRatio = 1.f + syncAmt * 3.f; // 1x to 4x
            syncPhase += phaseInc * syncRatio;

            // Reset on master phase wrap (phase < phaseInc means it just wrapped)
            if (phase < phaseInc)
                syncPhase = phase * syncRatio;

            syncPhase -= std::floor(syncPhase);

            // Simple sine sub-oscillator for sync
            float syncSample = std::sin(sdf::TWO_PI * syncPhase);
            sample = sample * (1.f - syncAmt) + syncSample * syncAmt;
        }

        return sample;
    }

private:
    float foldAmt = 0.f;
    float pdAmt = 0.f;
    float pwAmt = 0.5f;
    float syncAmt = 0.f;

    // ADAA state for wavefolding
    float adaaPrevX = 0.f;
    float adaaPrevF = 0.f;

    // Sync sub-oscillator phase
    float syncPhase = 0.f;
};
