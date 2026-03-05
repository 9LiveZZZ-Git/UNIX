"""
Scan Mode Tests - All 6 SDF Synth Scan Modes

Verifies each scan mode (Contour, RayMarch, Acoustic, Granular, Spectral,
Traverse) produces valid, distinct audio.
"""

import numpy as np
import pytest

from tests.tools.dawdreamer_host import DawDreamerHost
from tests.sdf_synth.conftest import (
    SCAN_MODES, set_scan_mode, set_param, norm_float,
)


SCAN_MODE_NAMES = list(SCAN_MODES.keys())


class TestContourMode:
    """Tests that run in the default Contour scan mode."""

    def test_contour_produces_audio(self, sdf_plugin: DawDreamerHost):
        """Contour mode (default) produces non-silent audio."""
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.5)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio
        rms = np.sqrt(np.mean(audio ** 2))
        print(f"\nContour mode RMS: {rms:.6f}")
        assert rms > 0.001, f"Contour mode produced silence (RMS={rms:.6f})"

    def test_contour_no_nan_inf(self, sdf_plugin: DawDreamerHost):
        """No NaN or Inf in Contour output."""
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.5)
        assert not np.any(np.isnan(result.audio)), "Contour produced NaN"
        assert not np.any(np.isinf(result.audio)), "Contour produced Inf"

    def test_contour_no_clipping(self, sdf_plugin: DawDreamerHost):
        """Peak amplitude stays below 1.0."""
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.5)
        peak = np.max(np.abs(result.audio))
        print(f"\nContour peak: {peak:.4f}")
        assert peak < 1.0, f"Contour clipped (peak={peak:.4f})"


class TestScanModeSwitch:
    """Tests that require changing scan mode."""

    @pytest.mark.parametrize("mode", SCAN_MODE_NAMES)
    def test_mode_produces_audio(self, sdf_plugin: DawDreamerHost, mode):
        """Each scan mode produces non-silent audio."""
        set_scan_mode(sdf_plugin, mode)
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.5)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio
        rms = np.sqrt(np.mean(audio ** 2))
        print(f"\n{mode} mode RMS: {rms:.6f}")
        assert rms > 0.001, f"{mode} mode produced silence (RMS={rms:.6f})"

    @pytest.mark.parametrize("mode", SCAN_MODE_NAMES)
    def test_mode_no_nan_inf(self, sdf_plugin: DawDreamerHost, mode):
        """Each scan mode produces no NaN or Inf."""
        set_scan_mode(sdf_plugin, mode)
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.5)
        assert not np.any(np.isnan(result.audio)), f"{mode} produced NaN"
        assert not np.any(np.isinf(result.audio)), f"{mode} produced Inf"

    @pytest.mark.parametrize("mode", SCAN_MODE_NAMES)
    def test_mode_no_clipping(self, sdf_plugin: DawDreamerHost, mode):
        """Each scan mode stays below clipping threshold."""
        set_scan_mode(sdf_plugin, mode)
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.5)
        peak = np.max(np.abs(result.audio))
        print(f"\n{mode} peak: {peak:.4f}")
        assert peak < 1.0, f"{mode} clipped (peak={peak:.4f})"

    def test_modes_produce_different_output(self, sdf_plugin: DawDreamerHost):
        """All 6 modes should yield pairwise-different waveforms."""
        waveforms = {}
        for mode in SCAN_MODE_NAMES:
            set_scan_mode(sdf_plugin, mode)
            result = sdf_plugin.render_note(note=60, velocity=100,
                                            duration_seconds=0.5,
                                            tail_seconds=0.0)
            audio = result.audio[0] if result.audio.ndim > 1 else result.audio
            waveforms[mode] = audio

        modes = list(waveforms.keys())
        high_corr_pairs = []
        for i in range(len(modes)):
            for j in range(i + 1, len(modes)):
                a = waveforms[modes[i]]
                b = waveforms[modes[j]]
                min_len = min(len(a), len(b))
                corr = np.corrcoef(a[:min_len], b[:min_len])[0, 1]
                print(f"  {modes[i]} vs {modes[j]}: corr={corr:.4f}")
                if abs(corr) > 0.99:
                    high_corr_pairs.append((modes[i], modes[j], corr))

        assert len(high_corr_pairs) == 0, (
            f"Modes too similar: {high_corr_pairs}"
        )


class TestContourTimbre:
    """Timbral tests that work with the default Contour mode."""

    def test_contour_default_sphere_harmonic_content(
            self, sdf_plugin: DawDreamerHost):
        """Contour mode on default Sphere should have a clear fundamental."""
        set_param(sdf_plugin, "shape1", 0.0)   # Sphere
        set_param(sdf_plugin, "operation", 0.0)  # SmoothUnion

        result = sdf_plugin.render_note(note=69, velocity=100,
                                        duration_seconds=1.0,
                                        tail_seconds=0.0)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        n = len(audio)
        fft = np.abs(np.fft.rfft(audio))
        freqs = np.fft.rfftfreq(n, d=1.0 / result.sample_rate)

        # Find fundamental peak near 440 Hz
        fund_idx = np.argmin(np.abs(freqs - 440))
        window = 5
        fund_power = np.sum(
            fft[max(0, fund_idx - window):fund_idx + window + 1] ** 2)
        total_power = np.sum(fft[1:] ** 2)  # Skip DC

        fund_ratio = fund_power / (total_power + 1e-20)
        print(f"\nFundamental energy ratio: {fund_ratio:.4f}")
        # Fundamental should be present
        assert fund_ratio > 0.01, (
            f"Fundamental too weak (ratio={fund_ratio:.4f})"
        )

    def test_topo_morph_changes_output(self, sdf_plugin: DawDreamerHost):
        """topoMorph=0 vs topoMorph=1 should produce different output."""
        set_param(sdf_plugin, "topoMorph", 0.0)
        r1 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a1 = r1.audio[0] if r1.audio.ndim > 1 else r1.audio

        set_param(sdf_plugin, "topoMorph", 1.0)
        r2 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a2 = r2.audio[0] if r2.audio.ndim > 1 else r2.audio

        min_len = min(len(a1), len(a2))
        corr = np.corrcoef(a1[:min_len], a2[:min_len])[0, 1]
        rms1 = np.sqrt(np.mean(a1 ** 2))
        rms2 = np.sqrt(np.mean(a2 ** 2))
        rms_diff = abs(rms1 - rms2)
        print(f"\ntopoMorph 0 vs 1: corr={corr:.4f}, RMS diff={rms_diff:.4f}")
        assert abs(corr) < 0.99 or rms_diff > 0.001, (
            f"topoMorph should change output (corr={corr:.4f})"
        )
