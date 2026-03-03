#pragma once
#include "Constants.h"
#include <cmath>
#include <algorithm>
#include <array>

// Forward declare to avoid circular includes
class WavetableGenerator;

struct MipMappedWavetable
{
    std::array<std::array<float, sdf::TABLE_SIZE>, sdf::MIP_LEVELS> tables;
    int numLevels = sdf::MIP_LEVELS;
};

class SDFOscillator
{
public:
    void setFrequency(float hz, float sampleRate)
    {
        phaseInc = hz / sampleRate;
    }

    void setWavetable(const float* table)
    {
        flatWavetable = table;
        mipTable = nullptr;
        prevMipTable = nullptr;
        crossfadeRemaining = 0;
    }

    void setMipMappedWavetable(const MipMappedWavetable* mip)
    {
        mipTable = mip;
        flatWavetable = nullptr;
    }

    // Set new mip table with crossfade from old (caller provides stashed old data)
    void crossfadeToMipTable(const MipMappedWavetable* newMip, const MipMappedWavetable* oldMip, int crossfadeSamples)
    {
        if (oldMip != nullptr && newMip != oldMip)
        {
            prevMipTable = oldMip;
            crossfadeRemaining = crossfadeSamples;
            crossfadeTotal = crossfadeSamples;
        }
        mipTable = newMip;
        flatWavetable = nullptr;
    }

    void reset()
    {
        phase = 0.f;
        prevMipTable = nullptr;
        crossfadeRemaining = 0;
        // Snap mip level to correct value to avoid initial aliasing
        if (phaseInc > 0.f)
            smoothedLevel = std::max(0.f, std::log2(phaseInc * sdf::TABLE_SIZE) + 0.5f);
        else
            smoothedLevel = 0.f;
    }

    float nextSample()
    {
        if (mipTable != nullptr)
            return nextSampleMip();

        if (flatWavetable != nullptr)
            return nextSampleFlat(flatWavetable);

        return 0.f;
    }

    // Overload: apply a phase distortion offset before lookup
    float nextSample(float phaseOffset)
    {
        float origPhase = phase;
        float modPhase = phase + phaseOffset;
        modPhase -= std::floor(modPhase);
        phase = modPhase;
        float sample = nextSampleNoAdvance();
        phase = origPhase;
        advancePhase();
        return sample;
    }

    // Read-only lookup at current phase without advancing
    float nextSampleNoAdvance() const
    {
        if (mipTable != nullptr)
        {
            float pos = phase * sdf::TABLE_SIZE;
            // Must use const version
            return const_cast<SDFOscillator*>(this)->sampleFromMip(mipTable, pos);
        }
        if (flatWavetable != nullptr)
        {
            float pos = phase * sdf::TABLE_SIZE;
            return interpolate(flatWavetable, pos);
        }
        return 0.f;
    }

    // Advance phase by one sample (separated for composability)
    void advancePhase()
    {
        phase += phaseInc;
        phase -= std::floor(phase);
    }

    float getPhase() const { return phase; }
    float getPhaseInc() const { return phaseInc; }

    void addPitchBend(float semitones, float sampleRate, float baseFreq)
    {
        float ratio = std::pow(2.f, semitones / 12.f);
        phaseInc = (baseFreq * ratio) / sampleRate;
    }

private:
    // Catmull-Rom cubic Hermite interpolation (4-point, 3rd-order)
    // Exact at sample points: f=0 → b, f=1 → c
    float interpolate(const float* table, float pos) const
    {
        int i0 = static_cast<int>(pos) % sdf::TABLE_SIZE;
        int im1 = (i0 - 1 + sdf::TABLE_SIZE) % sdf::TABLE_SIZE;
        int i1 = (i0 + 1) % sdf::TABLE_SIZE;
        int i2 = (i0 + 2) % sdf::TABLE_SIZE;
        float f = pos - std::floor(pos);

        float a = table[im1], b = table[i0];
        float c = table[i1], d = table[i2];

        float a0 = 0.5f * (-a + 3.f*b - 3.f*c + d);
        float a1 = 0.5f * (2.f*a - 5.f*b + 4.f*c - d);
        float a2 = 0.5f * (-a + c);
        float a3 = b;
        return ((a0 * f + a1) * f + a2) * f + a3;
    }

    float sampleFromMip(const MipMappedWavetable* table, float pos)
    {
        // Bias +0.5 so we always lean toward the more band-limited table,
        // eliminating aliasing at octave crossover boundaries
        float level = std::log2(phaseInc * sdf::TABLE_SIZE) + 0.5f;
        level = std::clamp(level, 0.f, static_cast<float>(table->numLevels - 2));
        // One-pole smoothing to prevent mip level jitter (fast: ~15ms settling)
        smoothedLevel += (level - smoothedLevel) * 0.3f;
        level = smoothedLevel;

        int levelI = static_cast<int>(level);
        float levelF = level - static_cast<float>(levelI);

        float s0 = interpolate(table->tables[levelI].data(), pos);
        float s1 = interpolate(table->tables[levelI + 1].data(), pos);
        return s0 + (s1 - s0) * levelF;
    }

    float nextSampleFlat(const float* table)
    {
        float pos = phase * sdf::TABLE_SIZE;
        float sample = interpolate(table, pos);

        phase += phaseInc;
        phase -= std::floor(phase);
        return sample;
    }

    float nextSampleMip()
    {
        float pos = phase * sdf::TABLE_SIZE;
        float sample = sampleFromMip(mipTable, pos);

        // Crossfade with previous table
        if (crossfadeRemaining > 0 && prevMipTable != nullptr)
        {
            --crossfadeRemaining;  // decrement first to fix off-by-one
            float prevSample = sampleFromMip(prevMipTable, pos);
            float t = static_cast<float>(crossfadeRemaining) / static_cast<float>(crossfadeTotal);
            sample = sample * (1.f - t) + prevSample * t;
            if (crossfadeRemaining <= 0)
                prevMipTable = nullptr;
        }

        phase += phaseInc;
        phase -= std::floor(phase);
        return sample;
    }

    float phase = 0.f;
    float phaseInc = 0.f;
    const float* flatWavetable = nullptr;
    const MipMappedWavetable* mipTable = nullptr;

    // Mip level hysteresis
    float smoothedLevel = 0.f;

    // Crossfade state
    const MipMappedWavetable* prevMipTable = nullptr;
    int crossfadeRemaining = 0;
    int crossfadeTotal = 64;
};
