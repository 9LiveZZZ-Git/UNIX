#pragma once
#include <cmath>
#include <random>
#include <algorithm>

enum class NoiseType { White = 0, Pink, Brown };

class NoiseGenerator
{
public:
    void reset()
    {
        rng.seed(42);
        brownState = 0.f;
        for (auto& r : pinkRows) r = 0.f;
        pinkRunningSum = 0.f;
        pinkIndex = 0;
    }

    void setSampleRate(float sr)
    {
        sampleRate = sr;
        // Brown noise leaky integrator: corner at ~2 Hz
        brownCoeff = 1.f - (6.28318530f * 2.f / sr);
        if (brownCoeff < 0.f) brownCoeff = 0.f;
    }

    float nextSample(NoiseType type)
    {
        switch (type)
        {
            case NoiseType::White: return white();
            case NoiseType::Pink:  return pink();
            case NoiseType::Brown: return brown();
            default:               return white();
        }
    }

private:
    // White noise: uniform [-1, 1]
    float white()
    {
        return dist(rng);
    }

    // Pink noise: Voss-McCartney 7-row algorithm (~-3dB/octave)
    float pink()
    {
        constexpr int NUM_ROWS = 7;
        int lastIndex = pinkIndex;
        ++pinkIndex;

        // XOR to find which rows changed
        int diff = lastIndex ^ pinkIndex;

        for (int r = 0; r < NUM_ROWS; ++r)
        {
            if (diff & (1 << r))
            {
                pinkRunningSum -= pinkRows[r];
                pinkRows[r] = dist(rng);
                pinkRunningSum += pinkRows[r];
            }
        }

        // Add white noise for high-frequency fill, normalize
        return (pinkRunningSum + dist(rng)) * (1.f / (NUM_ROWS + 1));
    }

    // Brown noise: leaky integrator of white noise
    float brown()
    {
        float w = dist(rng);
        brownState = brownCoeff * brownState + (1.f - brownCoeff) * w;
        return brownState;
    }

    std::minstd_rand rng{ 42 };
    std::uniform_real_distribution<float> dist{ -1.f, 1.f };
    float sampleRate = 44100.f;

    // Pink state (Voss-McCartney)
    float pinkRows[7] = {};
    float pinkRunningSum = 0.f;
    int pinkIndex = 0;

    // Brown state
    float brownState = 0.f;
    float brownCoeff = 0.9997f;
};
