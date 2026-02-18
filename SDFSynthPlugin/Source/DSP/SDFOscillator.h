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

    // Set new mip table with crossfade from old
    void crossfadeToMipTable(const MipMappedWavetable* newMip, int crossfadeSamples)
    {
        if (mipTable != nullptr && newMip != mipTable)
        {
            prevMipTable = mipTable;
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
    }

    float nextSample()
    {
        if (mipTable != nullptr)
            return nextSampleMip();

        if (flatWavetable != nullptr)
            return nextSampleFlat(flatWavetable);

        return 0.f;
    }

    float getPhase() const { return phase; }
    float getPhaseInc() const { return phaseInc; }

    void addPitchBend(float semitones, float sampleRate, float baseFreq)
    {
        float ratio = std::pow(2.f, semitones / 12.f);
        phaseInc = (baseFreq * ratio) / sampleRate;
    }

private:
    // Niemitalo optimal 4-point interpolation from a specific table
    float interpolate(const float* table, float pos) const
    {
        int i0 = static_cast<int>(pos) % sdf::TABLE_SIZE;
        int im1 = (i0 - 1 + sdf::TABLE_SIZE) % sdf::TABLE_SIZE;
        int i1 = (i0 + 1) % sdf::TABLE_SIZE;
        int i2 = (i0 + 2) % sdf::TABLE_SIZE;
        float f = pos - std::floor(pos);

        float a = table[im1], b = table[i0];
        float c = table[i1], d = table[i2];

        float even1 = a + d, odd1 = a - d;
        float even2 = b + c, odd2 = b - c;
        float c0 = even1 * -0.0018f + even2 *  0.5018f;
        float c1 = odd1  * -0.0900f + odd2  *  0.9900f;
        float c2 = even1 *  0.3200f + even2 * -0.3200f;
        float c3 = odd1  *  0.3900f + odd2  * -0.3900f;
        float c4 = even1 * -0.2300f + even2 *  0.2300f;
        return ((((c4 * f + c3) * f + c2) * f + c1) * f + c0);
    }

    float sampleFromMip(const MipMappedWavetable* table, float pos) const
    {
        float level = std::log2(phaseInc * sdf::TABLE_SIZE);
        level = std::clamp(level, 0.f, static_cast<float>(table->numLevels - 2));

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
            float prevSample = sampleFromMip(prevMipTable, pos);
            float t = static_cast<float>(crossfadeRemaining) / static_cast<float>(crossfadeTotal);
            sample = sample * (1.f - t) + prevSample * t;
            --crossfadeRemaining;
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

    // Crossfade state
    const MipMappedWavetable* prevMipTable = nullptr;
    int crossfadeRemaining = 0;
    int crossfadeTotal = 64;
};
