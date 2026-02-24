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

void SDFSynthesiser::setMipMappedWavetable(const MipMappedWavetable& mip, int crossfadeSamples)
{
    bool hadTable = (currentMipTable.numLevels > 0);
    currentMipTable = mip;
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SDFVoice*>(getVoice(i)))
        {
            if (hadTable && crossfadeSamples > 0 && voice->isVoiceActive())
                voice->crossfadeToMipTable(&currentMipTable, crossfadeSamples);
            else
                voice->setMipMappedWavetable(&currentMipTable);
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
    juce::SynthesiserSound* soundToPlay,
    int /*midiChannel*/, int midiNoteNumber) const
{
    // Scoring: lower score = better candidate to steal
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

        // Prefer voices in release (score 0) over sustaining (score 1000)
        if (sv->isInRelease())
            score += 0.f;
        else
            score += 1000.f;

        // Prefer same note (score 0) over different (score 500)
        if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
            score -= 500.f;

        // Prefer quieter voices
        score += sv->getVelocityGain() * 100.f;

        // Prefer older voices (lower index = allocated earlier, slight tiebreaker)
        score += static_cast<float>(i) * 0.1f;

        if (score < bestScore)
        {
            bestScore = score;
            best = voice;
        }
    }

    return best != nullptr ? best : getVoice(0);
}
