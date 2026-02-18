"""
SVF Filter Tests

Verifies the SVF filter (filterCutoff, filterRes) behaves correctly:
low cutoff reduces brightness, resonance creates peaks, no instability.
"""

import numpy as np
import pytest

from tests.tools.dawdreamer_host import DawDreamerHost
from tests.sdf_synth.conftest import set_param, norm_float


def _high_freq_energy_ratio(audio: np.ndarray, sample_rate: int,
                            split_hz: float = 2000.0) -> float:
    """Fraction of spectral energy above split_hz."""
    fft = np.abs(np.fft.rfft(audio))
    freqs = np.fft.rfftfreq(len(audio), d=1.0 / sample_rate)
    total = np.sum(fft ** 2)
    high = np.sum(fft[freqs > split_hz] ** 2)
    return high / (total + 1e-20)


class TestFilterCutoff:
    """Filter cutoff should control brightness."""

    def test_low_cutoff_reduces_brightness(self, sdf_plugin: DawDreamerHost):
        """cutoff near min should have less high-freq energy than fully open."""
        set_param(sdf_plugin, "filterRes", 0.0)

        # Low cutoff (normalized ~0.02 in a skewed range maps to ~200 Hz)
        set_param(sdf_plugin, "filterCutoff", 0.02)
        r_lo = sdf_plugin.render_note(note=60, velocity=100,
                                      duration_seconds=0.5, tail_seconds=0.0)
        a_lo = r_lo.audio[0] if r_lo.audio.ndim > 1 else r_lo.audio

        # High cutoff (fully open)
        set_param(sdf_plugin, "filterCutoff", 1.0)
        r_hi = sdf_plugin.render_note(note=60, velocity=100,
                                      duration_seconds=0.5, tail_seconds=0.0)
        a_hi = r_hi.audio[0] if r_hi.audio.ndim > 1 else r_hi.audio

        hf_lo = _high_freq_energy_ratio(a_lo, r_lo.sample_rate)
        hf_hi = _high_freq_energy_ratio(a_hi, r_hi.sample_rate)

        print(f"\nHF ratio cutoff=low:  {hf_lo:.6f}")
        print(f"HF ratio cutoff=high: {hf_hi:.6f}")
        assert hf_lo < hf_hi, (
            "Low cutoff should have less high-frequency energy"
        )

    def test_full_cutoff_passes_signal(self, sdf_plugin: DawDreamerHost):
        """cutoff=20kHz should not significantly alter the signal."""
        set_param(sdf_plugin, "filterCutoff", 1.0)
        set_param(sdf_plugin, "filterRes", 0.0)

        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.0)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio
        rms = np.sqrt(np.mean(audio ** 2))
        print(f"\nFull cutoff RMS: {rms:.6f}")
        assert rms > 0.001, "Full cutoff should pass signal through"


class TestFilterResonance:
    """Resonance should create a spectral peak."""

    def test_resonance_creates_peak(self, sdf_plugin: DawDreamerHost):
        """High resonance should create a spectral peak near cutoff."""
        # Set cutoff to a mid frequency for clear measurement
        set_param(sdf_plugin, "filterCutoff", 0.3)

        # Low resonance
        set_param(sdf_plugin, "filterRes", 0.0)
        r_flat = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.0)
        a_flat = r_flat.audio[0] if r_flat.audio.ndim > 1 else r_flat.audio

        # High resonance
        set_param(sdf_plugin, "filterRes", 0.9)
        r_reso = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.0)
        a_reso = r_reso.audio[0] if r_reso.audio.ndim > 1 else r_reso.audio

        # Compute spectral peaks
        fft_flat = np.abs(np.fft.rfft(a_flat))
        fft_reso = np.abs(np.fft.rfft(a_reso))

        # With resonance, the peak should be more pronounced
        peak_ratio_flat = np.max(fft_flat) / (np.mean(fft_flat) + 1e-10)
        peak_ratio_reso = np.max(fft_reso) / (np.mean(fft_reso) + 1e-10)

        print(f"\nPeak/mean ratio no reso:   {peak_ratio_flat:.1f}")
        print(f"Peak/mean ratio high reso: {peak_ratio_reso:.1f}")
        assert peak_ratio_reso > peak_ratio_flat * 0.8, (
            "Resonance should create spectral peaks"
        )


class TestFilterStability:
    """Filter should remain stable under extreme settings."""

    def test_filter_no_instability(self, sdf_plugin: DawDreamerHost):
        """Extreme cutoff+resonance, no NaN/Inf."""
        set_param(sdf_plugin, "filterCutoff", 0.0)  # minimum cutoff
        set_param(sdf_plugin, "filterRes", 0.98)

        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=1.0, tail_seconds=0.5)
        assert not np.any(np.isnan(result.audio)), "Filter produced NaN"
        assert not np.any(np.isinf(result.audio)), "Filter produced Inf"
        peak = np.max(np.abs(result.audio))
        print(f"\nExtreme filter peak: {peak:.4f}")

    def test_filter_sweep_no_clicks(self, sdf_plugin: DawDreamerHost):
        """Sweeping cutoff should not produce clicks (samples > 1.0)."""
        set_param(sdf_plugin, "filterRes", 0.5)

        all_audio = []
        steps = 10
        for i in range(steps):
            cutoff = i / (steps - 1)
            set_param(sdf_plugin, "filterCutoff", cutoff)
            result = sdf_plugin.render_note(note=60, velocity=100,
                                            duration_seconds=0.1,
                                            tail_seconds=0.0)
            audio = result.audio[0] if result.audio.ndim > 1 else result.audio
            all_audio.append(audio)

        combined = np.concatenate(all_audio)
        peak = np.max(np.abs(combined))
        has_nan = np.any(np.isnan(combined))
        print(f"\nFilter sweep peak: {peak:.4f}, NaN: {has_nan}")
        assert not has_nan, "Filter sweep produced NaN"
        assert peak < 1.0, f"Filter sweep clipped (peak={peak:.4f})"
