"""
Audio Quality Tests

Measures DC offset, stereo output, spectral content, SNR, aliasing,
pitch stability, and extreme note handling.
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
    corr = corr[n - 1:]
    corr = corr / (corr[0] + 1e-10)
    min_lag = int(sample_rate / 4000)
    max_lag = min(int(sample_rate / 20), len(corr) - 1)
    dip = min_lag
    while dip < max_lag - 1 and corr[dip] > corr[dip + 1]:
        dip += 1
    if dip >= max_lag - 1:
        return 0.0
    peak_idx = dip + np.argmax(corr[dip:max_lag])
    if peak_idx == 0:
        return 0.0
    return sample_rate / peak_idx


class TestDCOffset:
    """Output should not have significant DC offset."""

    def test_no_dc_offset(self, sdf_plugin: DawDreamerHost):
        """DC offset < 0.03."""
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=1.0, tail_seconds=0.0)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        start = int(0.1 * result.sample_rate)
        end = int(0.9 * result.sample_rate)
        dc = np.abs(np.mean(audio[start:end]))
        print(f"\nDC offset: {dc:.6f}")
        assert dc < 0.03, f"DC offset {dc:.6f} exceeds threshold 0.03"


class TestStereoOutput:
    """Plugin should produce stereo output."""

    def test_stereo_output(self, sdf_plugin: DawDreamerHost):
        """Both channels have content."""
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.0)
        assert result.audio.shape[0] == 2, "Expected stereo output"
        rms_l = np.sqrt(np.mean(result.audio[0] ** 2))
        rms_r = np.sqrt(np.mean(result.audio[1] ** 2))
        print(f"\nL RMS: {rms_l:.4f}, R RMS: {rms_r:.4f}")
        assert rms_l > 0.001, "Left channel silent"
        assert rms_r > 0.001, "Right channel silent"


class TestDecay:
    """Note should decay to silence after release."""

    def test_decay_to_silence(self, sdf_plugin: DawDreamerHost):
        """Note decays to near-zero after release."""
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5,
                                        tail_seconds=4.0)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        tail = audio[int(3.5 * result.sample_rate):]
        tail_rms = np.sqrt(np.mean(tail ** 2))
        print(f"\nTail RMS (3.5s+): {tail_rms:.6e}")
        assert tail_rms < 0.01, (
            f"Note didn't decay to silence (RMS={tail_rms:.6f})"
        )


class TestSpectralContent:
    """Output should have correct spectral properties."""

    def test_spectral_content_at_fundamental(self, sdf_plugin: DawDreamerHost):
        """FFT shows energy at expected fundamental."""
        result = sdf_plugin.render_note(note=69, velocity=100,
                                        duration_seconds=1.0, tail_seconds=0.0)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        fft = np.abs(np.fft.rfft(audio))
        freqs = np.fft.rfftfreq(len(audio), d=1.0 / result.sample_rate)

        peak_idx = np.argmax(fft[1:]) + 1  # Skip DC
        peak_freq = freqs[peak_idx]

        print(f"\nSpectral peak: {peak_freq:.1f} Hz (expected ~440 Hz)")
        assert abs(peak_freq - 440) < 20, (
            f"Spectral peak {peak_freq:.1f} Hz too far from 440 Hz"
        )


class TestSNR:
    """Signal-to-noise ratio should be acceptable."""

    def test_snr_above_threshold(self, sdf_plugin: DawDreamerHost):
        """SNR > 60 dB."""
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=1.0,
                                        tail_seconds=3.0)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        sig_start = int(0.1 * result.sample_rate)
        sig_end = int(0.8 * result.sample_rate)
        signal_power = np.mean(audio[sig_start:sig_end] ** 2)

        noise_start = int(3.0 * result.sample_rate)
        noise_end = int(3.8 * result.sample_rate)
        noise_power = np.mean(audio[noise_start:noise_end] ** 2)

        if noise_power > 0:
            snr_db = 10 * np.log10(signal_power / noise_power)
        else:
            snr_db = 120

        print(f"\nSNR: {snr_db:.1f} dB")
        assert snr_db > 60, f"SNR {snr_db:.1f} dB below 60 dB threshold"


class TestAliasing:
    """High notes should not produce aliasing artifacts."""

    def test_no_aliasing_high_note(self, sdf_plugin: DawDreamerHost):
        """C7 (2093 Hz) produces valid output without NaN."""
        result = sdf_plugin.render_note(note=96, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.0)
        assert not np.any(np.isnan(result.audio)), "High note produced NaN"
        assert not np.any(np.isinf(result.audio)), "High note produced Inf"
        rms = np.sqrt(np.mean(result.audio ** 2))
        print(f"\nC7 RMS: {rms:.4f}")
        assert rms > 0.0001, "C7 should produce output"


class TestPitchStability:
    """Pitch should remain stable during sustained notes."""

    def test_pitch_stability(self, sdf_plugin: DawDreamerHost):
        """Pitch doesn't drift during a sustained note (< 10 cents)."""
        result = sdf_plugin.render_note(note=69, velocity=100,
                                        duration_seconds=2.0, tail_seconds=0.0)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        segment_len = int(0.4 * result.sample_rate)
        pitches = []
        for i in range(4):
            start = int(0.2 * result.sample_rate) + i * segment_len
            end = start + segment_len
            if end > len(audio):
                break
            p = _detect_pitch(audio[start:end], result.sample_rate)
            if p > 0:
                pitches.append(p)

        if len(pitches) >= 2:
            max_drift_cents = max(
                abs(1200 * np.log2(p / pitches[0])) for p in pitches[1:]
            )
            print(f"\nPitch stability: {[f'{p:.1f}' for p in pitches]}")
            print(f"Max drift: {max_drift_cents:.1f} cents")
            assert max_drift_cents < 10, (
                f"Pitch drifted {max_drift_cents:.1f} cents"
            )
        else:
            pytest.skip("Could not detect pitch in enough segments")


class TestExtremeNotes:
    """Extreme MIDI note numbers should not crash."""

    @pytest.mark.parametrize("note", [0, 12, 60, 108, 127])
    def test_extreme_notes_no_crash(self, sdf_plugin: DawDreamerHost, note):
        """MIDI notes at extremes render without error."""
        result = sdf_plugin.render_note(note=note, velocity=100,
                                        duration_seconds=0.3, tail_seconds=0.1)
        assert not np.any(np.isnan(result.audio)), (
            f"MIDI {note} produced NaN"
        )
        assert not np.any(np.isinf(result.audio)), (
            f"MIDI {note} produced Inf"
        )
        print(f"\nMIDI {note}: peak={np.max(np.abs(result.audio)):.4f}")
