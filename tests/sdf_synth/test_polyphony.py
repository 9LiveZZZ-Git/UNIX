"""
Polyphony Tests

Verifies multi-voice behavior: chords are louder, 16 voices don't crash,
voice stealing doesn't cause silence.
"""

import numpy as np
import pytest

from tests.tools.dawdreamer_host import DawDreamerHost, MIDIEvent


class TestBasicPolyphony:
    """Core polyphony functionality."""

    def test_single_note_clean(self, sdf_plugin: DawDreamerHost):
        """Single note produces clean audio."""
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.5)
        rms = np.sqrt(np.mean(result.audio ** 2))
        print(f"\nSingle note RMS: {rms:.4f}")
        assert rms > 0.001, "Single note should produce audio"
        assert not np.any(np.isnan(result.audio)), "Single note NaN"

    def test_chord_louder_than_single(self, sdf_plugin: DawDreamerHost):
        """3-note chord should have higher RMS than single note."""
        single = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.0)
        single_rms = np.sqrt(np.mean(single.audio ** 2))

        chord = sdf_plugin.render_chord(notes=[60, 64, 67], velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.0)
        chord_rms = np.sqrt(np.mean(chord.audio ** 2))

        print(f"\nSingle RMS: {single_rms:.4f}")
        print(f"Chord RMS:  {chord_rms:.4f}")
        print(f"Ratio: {chord_rms / (single_rms + 1e-10):.2f}x")
        assert chord_rms > single_rms, "Chord should be louder than single note"

    def test_16_voices_no_crash(self, sdf_plugin: DawDreamerHost):
        """16 simultaneous voices render without NaN/Inf."""
        result = sdf_plugin.render_polyphony_test(
            num_voices=16, base_note=36, velocity=100,
            stagger_ms=5, hold_seconds=1.0)

        assert not np.any(np.isnan(result.audio)), "16 voices produced NaN"
        assert not np.any(np.isinf(result.audio)), "16 voices produced Inf"
        rms = np.sqrt(np.mean(result.audio ** 2))
        print(f"\n16-voice RMS: {rms:.4f}")
        assert rms > 0.001, "16 voices should produce audio"

    def test_16_voices_realtime(self, sdf_plugin: DawDreamerHost):
        """16 voices should render faster than realtime."""
        result = sdf_plugin.render_polyphony_test(
            num_voices=16, base_note=36, velocity=100,
            stagger_ms=5, hold_seconds=1.0)

        print(f"\n16-voice realtime ratio: {result.realtime_ratio:.4f}")
        assert result.realtime_ratio < 1.0, (
            f"16 voices too slow ({result.realtime_ratio:.2f}x realtime)"
        )


class TestPitchPolyphony:
    """Polyphony with distinct pitches."""

    def test_different_pitches_simultaneous(self, sdf_plugin: DawDreamerHost):
        """C3+E3+G3 chord: all three pitches detectable in spectrum."""
        # C3=48 (~130.8 Hz), E3=52 (~164.8 Hz), G3=55 (~196.0 Hz)
        chord = sdf_plugin.render_chord(notes=[48, 52, 55], velocity=100,
                                        duration_seconds=1.0, tail_seconds=0.0)
        audio = chord.audio[0] if chord.audio.ndim > 1 else chord.audio

        n = len(audio)
        fft = np.abs(np.fft.rfft(audio))
        freqs = np.fft.rfftfreq(n, d=1.0 / chord.sample_rate)

        expected = [130.8, 164.8, 196.0]
        for freq in expected:
            idx = np.argmin(np.abs(freqs - freq))
            window = 5
            energy = np.sum(fft[max(0, idx - window):idx + window + 1] ** 2)
            total = np.sum(fft ** 2)
            ratio = energy / (total + 1e-20)
            print(f"  {freq:.1f} Hz energy ratio: {ratio:.4f}")
            assert ratio > 0.001, (
                f"Pitch {freq:.1f} Hz not detected in chord"
            )


class TestVoiceStealing:
    """Voice stealing under overload."""

    def test_voice_stealing_no_silence(self, sdf_plugin: DawDreamerHost):
        """Play 20 notes (exceeds 16 voice limit), audio should not go silent."""
        events = []

        # Play 20 notes staggered by 50ms, each held for 2 seconds
        for i in range(20):
            t = i * 0.05
            note = 48 + (i % 12)
            events.append(MIDIEvent.note_on(t, note, 100))
            events.append(MIDIEvent.note_off(t + 2.0, note))

        result = sdf_plugin.render(duration_seconds=3.0, midi_events=events)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        # Check audio is non-silent during the sustained section
        start = int(1.0 * result.sample_rate)
        end = int(2.0 * result.sample_rate)
        sustained_rms = np.sqrt(np.mean(audio[start:end] ** 2))
        print(f"\nSustained 20-note RMS: {sustained_rms:.4f}")
        assert sustained_rms > 0.001, (
            "Audio went silent during voice stealing"
        )
