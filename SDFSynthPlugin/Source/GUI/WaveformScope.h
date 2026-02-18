#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "DSP/WavetableGenerator.h"
#include "DSP/ContourExtractor.h"
#include "Utility/Constants.h"

class WaveformScope : public juce::Component, public juce::Timer
{
public:
    WaveformScope();
    ~WaveformScope() override;

    void paint(juce::Graphics&) override;
    void timerCallback() override;

    void setWavetable(const WavetableGenerator::Wavetable* table) { wavetable = table; }
    void setContour(const std::vector<ContourPoint>* c) { contour = c; }
    void setPlayheadPhase(float p) { playheadPhase = p; }
    void setScanHeight(float h) { scanHeight = h; }
    void setTopoMorph(float m) { topoMorph = m; }
    void setScanMode(int m) { scanMode = m; }

private:
    const WavetableGenerator::Wavetable* wavetable = nullptr;
    const std::vector<ContourPoint>* contour = nullptr;
    float playheadPhase = -1.f;
    float scanHeight = 0.f;
    float topoMorph = 1.f;
    int scanMode = 0;

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> area);
    void drawWaveform(juce::Graphics& g, juce::Rectangle<float> area);
    void drawMiniCrossSection(juce::Graphics& g, juce::Rectangle<float> area);
    void drawHeightBar(juce::Graphics& g, juce::Rectangle<float> area);
    void drawMRIIndicator(juce::Graphics& g, juce::Rectangle<float> area);
    void drawPlayhead(juce::Graphics& g, juce::Rectangle<float> area);
};
