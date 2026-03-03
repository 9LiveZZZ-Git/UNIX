#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

enum class LFOShape { Sine, Triangle, Saw, Square, SandH, Random };

class LFO
{
public:
    void reset()
    {
        phase = 0.0;
        currentValue = 0.f;
        prevRandom = 0.f;
        nextRandom = randFloat();
        smoothRandom = 0.f;
    }

    void setRate(float hz) { rateHz = hz; }
    void setShape(LFOShape s) { shape = s; }
    void setPhaseOffset(float degrees) { phaseOffset = degrees; }
    void setSyncEnabled(bool sync) { syncEnabled = sync; }
    void setTempoSyncDivision(int div) { syncDivision = std::clamp(div, 0, 9); }

    // Block-rate: advance by numSamples, return current value
    float advance(int numSamples, double sampleRate, double bpm = 120.0)
    {
        if (sampleRate <= 0.0 || numSamples <= 0)
            return currentValue;

        double freq;
        if (syncEnabled)
            freq = bpmToHz(bpm, syncDivision);
        else
            freq = static_cast<double>(rateHz);

        double dt = freq / sampleRate;
        phase += dt * numSamples;

        // Check for cycle wrap (for S&H)
        bool wrapped = (phase >= 1.0);
        phase -= std::floor(phase);

        // Apply phase offset
        double p = phase + static_cast<double>(phaseOffset) / 360.0;
        p -= std::floor(p);

        switch (shape)
        {
            case LFOShape::Sine:
                currentValue = static_cast<float>(std::sin(p * 6.283185307179586));
                break;

            case LFOShape::Triangle:
                currentValue = static_cast<float>(4.0 * std::abs(p - 0.5) - 1.0);
                break;

            case LFOShape::Saw:
            {
                // PolyBLEP-corrected saw
                float naive = static_cast<float>(2.0 * p - 1.0);
                currentValue = naive - polyBLEP(p, dt);
                break;
            }

            case LFOShape::Square:
            {
                // PolyBLEP-corrected square
                float naive = (p < 0.5) ? 1.f : -1.f;
                currentValue = naive - polyBLEP(p, dt) + polyBLEP(std::fmod(p + 0.5, 1.0), dt);
                break;
            }

            case LFOShape::SandH:
                if (wrapped)
                {
                    prevRandom = nextRandom;
                    nextRandom = randFloat();
                }
                currentValue = prevRandom;
                break;

            case LFOShape::Random:
            {
                // Smoothed random walk
                if (wrapped)
                {
                    prevRandom = nextRandom;
                    nextRandom = randFloat();
                }
                // Interpolate between prev and next
                float frac = static_cast<float>(p);
                // Smoothstep interpolation
                float t = frac * frac * (3.f - 2.f * frac);
                smoothRandom = prevRandom + t * (nextRandom - prevRandom);
                currentValue = smoothRandom;
                break;
            }
        }

        return currentValue;
    }

    float getValue() const { return currentValue; }

    // Tempo sync rate table: index -> musical division
    // 0=8bars, 1=4bars, 2=2bars, 3=1bar, 4=1/2, 5=1/4, 6=1/8, 7=1/16, 8=1/32, 9=1/64
    static constexpr int NUM_SYNC_DIVISIONS = 10;

    static const char* getSyncDivisionName(int div)
    {
        static const char* names[] = {
            "8 Bars", "4 Bars", "2 Bars", "1 Bar",
            "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"
        };
        if (div >= 0 && div < NUM_SYNC_DIVISIONS)
            return names[div];
        return "1/4";
    }

private:
    double phase = 0.0;
    float currentValue = 0.f;
    float rateHz = 1.f;
    float phaseOffset = 0.f;
    LFOShape shape = LFOShape::Sine;
    bool syncEnabled = false;
    int syncDivision = 5; // default 1/4

    float prevRandom = 0.f;
    float nextRandom = 0.f;
    float smoothRandom = 0.f;

    // Simple LCG-based random for deterministic behavior
    uint32_t rngState = 12345;

    float randFloat()
    {
        rngState = rngState * 1664525u + 1013904223u;
        // Map to -1..+1
        return static_cast<float>(rngState) / 2147483648.f - 1.f;
    }

    // Convert BPM + sync division to Hz
    static double bpmToHz(double bpm, int division)
    {
        // Beats per bar (assuming 4/4)
        // Division: bars multiplied by beats
        static const double beatsPerDiv[] = {
            32.0, 16.0, 8.0, 4.0,   // 8bars, 4bars, 2bars, 1bar
            2.0, 1.0, 0.5, 0.25,    // 1/2, 1/4, 1/8, 1/16
            0.125, 0.0625           // 1/32, 1/64
        };
        if (division < 0 || division >= NUM_SYNC_DIVISIONS)
            division = 5;
        double beatsPerCycle = beatsPerDiv[division];
        double beatsPerSecond = bpm / 60.0;
        return beatsPerSecond / beatsPerCycle;
    }

    // PolyBLEP anti-aliasing correction
    static float polyBLEP(double t, double dt)
    {
        if (dt <= 0.0) return 0.f;
        if (t < dt)
        {
            double x = t / dt;
            return static_cast<float>(2.0 * x - x * x - 1.0);
        }
        else if (t > 1.0 - dt)
        {
            double x = (t - 1.0) / dt;
            return static_cast<float>(x * x + 2.0 * x + 1.0);
        }
        return 0.f;
    }
};
