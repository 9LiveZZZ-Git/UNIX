#include "SDFSynthesiser.h"

SDFSynthesiser::SDFSynthesiser()
{
    addSound(new SDFSound());

    for (int i = 0; i < sdf::MAX_VOICES; ++i)
        addVoice(new SDFVoice());
}

void SDFSynthesiser::setWavetable(const WavetableGenerator::Wavetable& table)
{
    currentTable = table;
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SDFVoice*>(getVoice(i)))
            voice->setWavetable(currentTable.data());
    }
}

void SDFSynthesiser::setMipMappedWavetable(std::shared_ptr<const MipMappedWavetable> mip, int crossfadeSamples)
{
    bool hadTable = (currentMipTable && currentMipTable->numLevels > 0);
    // Shift history: two-slot keeps old tables alive while voices crossfade
    prevPrevMipTable = std::move(prevMipTable);
    prevMipTable = currentMipTable;  // shared_ptr copy (8 bytes, no data copy)
    currentMipTable = std::move(mip);
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SDFVoice*>(getVoice(i)))
        {
            if (hadTable && crossfadeSamples > 0 && voice->isVoiceActive())
                voice->crossfadeToMipTable(currentMipTable.get(), prevMipTable.get(), crossfadeSamples);
            else
                voice->setMipMappedWavetable(currentMipTable.get());
        }
    }
}

void SDFSynthesiser::setMipMappedWavetableB(std::shared_ptr<const MipMappedWavetable> mip, int crossfadeSamples)
{
    bool hadTable = (currentMipTableB && currentMipTableB->numLevels > 0);
    prevPrevMipTableB = std::move(prevMipTableB);
    prevMipTableB = currentMipTableB;
    currentMipTableB = std::move(mip);
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SDFVoice*>(getVoice(i)))
        {
            if (hadTable && crossfadeSamples > 0 && voice->isVoiceActive())
                voice->crossfadeToMipTableB(currentMipTableB.get(), prevMipTableB.get(), crossfadeSamples);
            else
                voice->setMipMappedWavetableB(currentMipTableB.get());
        }
    }
}

void SDFSynthesiser::updateADSR(float a, float d, float s, float r)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SDFVoice*>(getVoice(i)))
            voice->setADSR(a, d, s, r);
    }
}

void SDFSynthesiser::updateOscEffects(float fold, float pd, float pw, float sync)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SDFVoice*>(getVoice(i)))
            voice->setOscEffectParams(fold, pd, pw, sync);
    }
}

void SDFSynthesiser::updateOscBParams(bool enable, float level, int semi, float fine, int mixMode, float fmDepth)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SDFVoice*>(getVoice(i)))
            voice->setOscBParams(enable, level, semi, fine, mixMode, fmDepth);
    }
}

void SDFSynthesiser::updateUnisonParams(int numUnison, float detune, float spread, float blend)
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SDFVoice*>(getVoice(i)))
            voice->setUnisonParams(numUnison, detune, spread, blend);
    }
}

float SDFSynthesiser::getActivePhase() const
{
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<const SDFVoice*>(getVoice(i)))
        {
            if (voice->isVoiceActive())
                return voice->getPhase();
        }
    }
    return -1.f;
}

int SDFSynthesiser::getActiveVoiceCount() const
{
    int count = 0;
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (getVoice(i)->isVoiceActive())
            ++count;
    }
    return count;
}

float SDFSynthesiser::getMaxEnvelopeValue() const
{
    float maxEnv = 0.f;
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<const SDFVoice*>(getVoice(i)))
        {
            if (voice->isVoiceActive())
                maxEnv = std::max(maxEnv, voice->getCurrentEnvelopeValue());
        }
    }
    return maxEnv;
}

juce::SynthesiserVoice* SDFSynthesiser::findVoiceToSteal(
    juce::SynthesiserSound* /*soundToPlay*/,
    int /*midiChannel*/, int midiNoteNumber) const
{
    juce::SynthesiserVoice* best = nullptr;
    float bestScore = 1e9f;

    for (int i = 0; i < getNumVoices(); ++i)
    {
        auto* voice = getVoice(i);
        if (!voice->isVoiceActive())
            continue;

        auto* sv = dynamic_cast<SDFVoice*>(voice);
        if (sv == nullptr)
            continue;

        float score = 0.f;

        if (sv->isInRelease())
            score += 0.f;
        else
            score += 1000.f;

        if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
            score -= 500.f;

        score += sv->getVelocityGain() * 100.f;
        score += static_cast<float>(i) * 0.1f;

        if (score < bestScore)
        {
            bestScore = score;
            best = voice;
        }
    }

    return best != nullptr ? best : getVoice(0);
}
