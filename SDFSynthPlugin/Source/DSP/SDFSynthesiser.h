#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "SDFVoice.h"
#include "WavetableGenerator.h"
#include "Constants.h"

class SDFSynthesiser : public juce::Synthesiser
{
public:
    SDFSynthesiser();

    void setWavetable(const WavetableGenerator::Wavetable& table);
    void setMipMappedWavetable(const MipMappedWavetable& mip, int crossfadeSamples = 0);
    void updateADSR(float a, float d, float s, float r);
    float getActivePhase() const;
    int getActiveVoiceCount() const;
    const MipMappedWavetable& getCurrentMipTable() const { return currentMipTable; }

protected:
    juce::SynthesiserVoice* findVoiceToSteal(juce::SynthesiserSound* soundToPlay,
                                              int midiChannel, int midiNoteNumber) const override;

private:
    WavetableGenerator::Wavetable currentTable{};
    MipMappedWavetable currentMipTable{};
};
