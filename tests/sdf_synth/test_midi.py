"""
MIDI Response Tests

Verifies correct MIDI handling: velocity response, pitch accuracy,
chromatic scale, octave relationships, note-off release.
"""

import numpy as np
import pytest

from tests.tools.dawdreamer_host import DawDreamerHost, MIDIEvent


def _detect_pitch(audio: np.ndarray, sample_rate: int) -> float:
    """Detect fundamental pitch using autocorrelation."""
    audio = audio - np.mean(audio)
    if np.max(np.abs(audio)) < 1e-6:
        return 0.0

    n = len(audio)
    corr = np.correlate(audio, audio, mode='full')
    corr = corr[n - 1:]  # Keep positive lags only
    corr = corr / (corr[0] + 1e-10)

    min_lag = int(sample_rate / 4000)  # 4 kHz max
    max_lag = int(sample_rate / 20)     # 20 Hz min

    if max_lag >= len(corr):
        max_lag = len(corr) - 1

    # Find first dip
    dip = min_lag
    while dip < max_lag - 1 and corr[dip] > corr[dip + 1]:
        dip += 1

    if dip >= max_lag - 1:
        return 0.0

    # Find peak after dip
    search_end = min(max_lag, len(corr))
    peak_idx = dip + np.argmax(corr[dip:search_end])
    if peak_idx == 0:
        return 0.0

    return sample_rate / peak_idx


def _midi_to_freq(note: int) -> float:
    """Convert MIDI note number to frequency in Hz."""
    return 440.0 * 2.0 ** ((note - 69) / 12.0)


def _freq_to_cents(f1: float, f2: float) -> float:
    """Absolute difference in cents between two frequencies."""
    if f1 <= 0 or f2 <= 0:
        return float('inf')
    return abs(1200 * np.log2(f2 / f1))


class TestVelocityResponse:
    """Velocity should affect output amplitude."""

    def test_velocity_affects_amplitude(self, sdf_plugin: DawDreamerHost):
        """velocity=127 louder than velocity=30."""
        result_loud = sdf_plugin.render_note(note=60, velocity=127,
                                             duration_seconds=0.5,
                                             tail_seconds=0.0)
        rms_loud = np.sqrt(np.mean(result_loud.audio ** 2))

        result_quiet = sdf_plugin.render_note(note=60, velocity=30,
                                              duration_seconds=0.5,
                                              tail_seconds=0.0)
        rms_quiet = np.sqrt(np.mean(result_quiet.audio ** 2))

        print(f"\nVelocity 127 RMS: {rms_loud:.4f}")
        print(f"Velocity 30  RMS: {rms_quiet:.4f}")
        assert rms_loud > rms_quiet, "Higher velocity should be louder"

    def test_velocity_zero_is_silent(self, sdf_plugin: DawDreamerHost):
        """velocity=0 should produce no audio."""
        result = sdf_plugin.render_note(note=60, velocity=0,
                                        duration_seconds=0.5, tail_seconds=0.0)
        rms = np.sqrt(np.mean(result.audio ** 2))
        print(f"\nVelocity 0 RMS: {rms:.6e}")
        assert rms < 0.001, f"Velocity 0 not silent (RMS={rms:.6f})"


class TestPitchAccuracy:
    """Synthesizer should produce correct pitches."""

    def test_pitch_accuracy_a440(self, sdf_plugin: DawDreamerHost):
        """MIDI 69 produces ~440Hz (within 50 cents)."""
        result = sdf_plugin.render_note(note=69, velocity=100,
                                        duration_seconds=1.0, tail_seconds=0.0)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        start = int(0.1 * result.sample_rate)
        end = int(0.9 * result.sample_rate)
        detected = _detect_pitch(audio[start:end], result.sample_rate)

        cents_off = _freq_to_cents(440.0, detected)
        print(f"\nA440 test: detected {detected:.1f} Hz "
              f"({cents_off:.1f} cents off)")
        assert cents_off < 50, (
            f"Pitch {detected:.1f} Hz is {cents_off:.1f} cents from 440 Hz"
        )

    def test_chromatic_scale(self, sdf_plugin: DawDreamerHost):
        """12 notes (C4-B4) all produce correct pitches within tolerance."""
        for note in range(60, 72):  # C4 to B4
            expected = _midi_to_freq(note)
            result = sdf_plugin.render_note(note=note, velocity=100,
                                            duration_seconds=0.5,
                                            tail_seconds=0.0)
            audio = result.audio[0] if result.audio.ndim > 1 else result.audio

            start = int(0.05 * result.sample_rate)
            end = int(0.45 * result.sample_rate)
            detected = _detect_pitch(audio[start:end], result.sample_rate)

            cents_off = _freq_to_cents(expected, detected)
            print(f"  MIDI {note}: expected {expected:.1f} Hz, "
                  f"got {detected:.1f} Hz ({cents_off:.1f} cents)")
            assert cents_off < 50, (
                f"MIDI {note}: {detected:.1f} Hz is {cents_off:.1f} cents "
                f"from {expected:.1f} Hz"
            )

    def test_octave_relationship(self, sdf_plugin: DawDreamerHost):
        """C3 vs C4 frequency ratio should be ~2.0."""
        # Render C4 first to avoid release-tail contamination from C3
        r4 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=1.0, tail_seconds=0.0)
        a4 = r4.audio[0] if r4.audio.ndim > 1 else r4.audio

        r3 = sdf_plugin.render_note(note=48, velocity=100,
                                    duration_seconds=1.0, tail_seconds=0.0)
        a3 = r3.audio[0] if r3.audio.ndim > 1 else r3.audio

        # Use FFT-based peak detection (more robust than autocorrelation)
        def fft_pitch(audio, sr):
            fft = np.abs(np.fft.rfft(audio))
            freqs = np.fft.rfftfreq(len(audio), d=1.0 / sr)
            # Search above 20 Hz, skip DC
            mask = freqs > 20
            peak_idx = np.argmax(fft[mask])
            return freqs[mask][peak_idx]

        start = int(0.1 * r3.sample_rate)
        end = int(0.9 * r3.sample_rate)
        f3 = fft_pitch(a3[start:end], r3.sample_rate)
        f4 = fft_pitch(a4[start:end], r4.sample_rate)

        ratio = f4 / (f3 + 1e-10)
        print(f"\nC3={f3:.1f} Hz, C4={f4:.1f} Hz, ratio={ratio:.3f}")
        assert abs(ratio - 2.0) < 0.15, (
            f"Octave ratio {ratio:.3f} too far from 2.0"
        )


class TestNoteOff:
    """Note-off should trigger release envelope."""

    def test_note_off_triggers_release(self, sdf_plugin: DawDreamerHost):
        """Audio decays after note-off (end RMS < 10% of start)."""
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5,
                                        tail_seconds=2.0)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        # RMS during sustained note
        start_rms = np.sqrt(np.mean(
            audio[int(0.1 * result.sample_rate):
                  int(0.4 * result.sample_rate)] ** 2
        ))

        # RMS well after note-off
        end_rms = np.sqrt(np.mean(
            audio[int(2.0 * result.sample_rate):
                  int(2.4 * result.sample_rate)] ** 2
        ))

        ratio = end_rms / (start_rms + 1e-10)
        print(f"\nSustained RMS: {start_rms:.4f}")
        print(f"Post-release RMS: {end_rms:.4f}")
        print(f"Ratio: {ratio:.4f}")
        assert ratio < 0.1, (
            f"Note didn't decay after release (ratio={ratio:.4f})"
        )
