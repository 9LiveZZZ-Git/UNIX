#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "SDFVoice.h"
#include "WavetableGenerator.h"
#include "Constants.h"
#include <memory>

class SDFSynthesiser : public juce::Synthesiser
{
public:
    SDFSynthesiser();

    void setWavetable(const WavetableGenerator::Wavetable& table);
    void setMipMappedWavetable(std::shared_ptr<const MipMappedWavetable> mip, int crossfadeSamples = 0);
    void setMipMappedWavetableB(std::shared_ptr<const MipMappedWavetable> mip, int crossfadeSamples = 0);
    void updateADSR(float a, float d, float s, float r);
    void updateOscEffects(float fold, float pd, float pw, float sync);
    void updateOscBParams(bool enable, float level, int semi, float fine, int mixMode, float fmDepth);
    void updateUnisonParams(int voices, float detune, float spread, float blend);
    float getActivePhase() const;
    int getActiveVoiceCount() const;
    float getMaxEnvelopeValue() const;
    const MipMappedWavetable* getCurrentMipTable() const { return currentMipTable.get(); }

protected:
    juce::SynthesiserVoice* findVoiceToSteal(juce::SynthesiserSound* soundToPlay,
                                              int midiChannel, int midiNoteNumber) const override;

private:
    WavetableGenerator::Wavetable currentTable{};
    // shared_ptr ownership: background thread allocates, audio thread swaps pointers (zero-copy)
    // Two-slot history keeps old tables alive while voices crossfade from them
    std::shared_ptr<const MipMappedWavetable> currentMipTable;
    std::shared_ptr<const MipMappedWavetable> prevMipTable;
    std::shared_ptr<const MipMappedWavetable> prevPrevMipTable;
    std::shared_ptr<const MipMappedWavetable> currentMipTableB;
    std::shared_ptr<const MipMappedWavetable> prevMipTableB;
    std::shared_ptr<const MipMappedWavetable> prevPrevMipTableB;
};
