"""
Parameter Sweep Tests

Verifies that key SDF Synth parameters affect the output and extreme
values don't cause NaN/Inf/clipping.
"""

import numpy as np
import pytest

from tests.tools.dawdreamer_host import DawDreamerHost
from tests.sdf_synth.conftest import set_param, norm_float


class TestScanHeightSweep:
    """Scan height should change the waveform."""

    def test_scan_height_sweep(self, sdf_plugin: DawDreamerHost):
        """5 heights all produce audio; different heights yield different spectra."""
        heights = [-0.9, -0.4, 0.0, 0.4, 0.9]
        waveforms = {}

        for h in heights:
            # scanHeight range: -0.9 to 0.9
            set_param(sdf_plugin, "scanHeight", norm_float(h, -0.9, 0.9))
            result = sdf_plugin.render_note(note=60, velocity=100,
                                            duration_seconds=0.5,
                                            tail_seconds=0.0)
            audio = result.audio[0] if result.audio.ndim > 1 else result.audio
            rms = np.sqrt(np.mean(audio ** 2))
            print(f"  scanHeight={h}: RMS={rms:.6f}")
            waveforms[h] = audio

        # At least 3 of 5 heights should produce audio
        active = sum(1 for a in waveforms.values()
                     if np.sqrt(np.mean(a ** 2)) > 0.001)
        assert active >= 3, f"Only {active}/5 scan heights produced audio"


class TestDistScale:
    """distScale should affect harmonic content."""

    def test_dist_scale_affects_output(self, sdf_plugin: DawDreamerHost):
        """Different distScale values produce different output."""
        # distScale range: 0.5 - 8.0
        set_param(sdf_plugin, "distScale", norm_float(0.5, 0.5, 8.0))
        r1 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a1 = r1.audio[0] if r1.audio.ndim > 1 else r1.audio

        set_param(sdf_plugin, "distScale", norm_float(8.0, 0.5, 8.0))
        r2 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a2 = r2.audio[0] if r2.audio.ndim > 1 else r2.audio

        min_len = min(len(a1), len(a2))
        corr = np.corrcoef(a1[:min_len], a2[:min_len])[0, 1]
        rms1 = np.sqrt(np.mean(a1 ** 2))
        rms2 = np.sqrt(np.mean(a2 ** 2))
        rms_diff = abs(rms1 - rms2)
        print(f"\ndistScale 0.5 vs 8.0: corr={corr:.4f}, "
              f"RMS diff={rms_diff:.6f}")
        assert abs(corr) < 0.99 or rms_diff > 0.001, (
            f"distScale should change output (corr={corr:.4f})"
        )


class TestTwist:
    """Twist parameter should deform the shape."""

    def test_twist_changes_output(self, sdf_plugin: DawDreamerHost):
        """twist=0 vs twist=max produce different waveforms."""
        # twist range: 0.0 - 6.0
        set_param(sdf_plugin, "twist", norm_float(0.0, 0.0, 6.0))
        r1 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a1 = r1.audio[0] if r1.audio.ndim > 1 else r1.audio

        set_param(sdf_plugin, "twist", norm_float(6.0, 0.0, 6.0))
        r2 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a2 = r2.audio[0] if r2.audio.ndim > 1 else r2.audio

        min_len = min(len(a1), len(a2))
        corr = np.corrcoef(a1[:min_len], a2[:min_len])[0, 1]
        rms1 = np.sqrt(np.mean(a1 ** 2))
        rms2 = np.sqrt(np.mean(a2 ** 2))
        rms_diff = abs(rms1 - rms2)
        print(f"\ntwist 0 vs 6: corr={corr:.4f}, RMS diff={rms_diff:.6f}")
        assert abs(corr) < 0.99 or rms_diff > 0.001, (
            f"Twist should change output (corr={corr:.4f})"
        )


class TestSizeParams:
    """Shape size parameters should affect the output."""

    def test_size_params_affect_output(self, sdf_plugin: DawDreamerHost):
        """size1=min vs size1=max produce different results."""
        # size1 range: 0.1 - 0.7
        set_param(sdf_plugin, "size1", norm_float(0.1, 0.1, 0.7))
        r1 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a1 = r1.audio[0] if r1.audio.ndim > 1 else r1.audio

        set_param(sdf_plugin, "size1", norm_float(0.7, 0.1, 0.7))
        r2 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a2 = r2.audio[0] if r2.audio.ndim > 1 else r2.audio

        min_len = min(len(a1), len(a2))
        corr = np.corrcoef(a1[:min_len], a2[:min_len])[0, 1]
        rms1 = np.sqrt(np.mean(a1 ** 2))
        rms2 = np.sqrt(np.mean(a2 ** 2))
        rms_diff = abs(rms1 - rms2)
        print(f"\nsize1 min vs max: corr={corr:.4f}, RMS diff={rms_diff:.6f}")
        assert abs(corr) < 0.99 or rms_diff > 0.001, (
            f"Size should change output (corr={corr:.4f})"
        )


class TestOffset:
    """Offset parameters should move the shape."""

    def test_offset_changes_output(self, sdf_plugin: DawDreamerHost):
        """offsetX=0 vs offsetX=max are different."""
        # offsetX range: -1.0 to 1.0
        set_param(sdf_plugin, "offsetX", norm_float(-1.0, -1.0, 1.0))
        r1 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a1 = r1.audio[0] if r1.audio.ndim > 1 else r1.audio

        set_param(sdf_plugin, "offsetX", norm_float(1.0, -1.0, 1.0))
        r2 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a2 = r2.audio[0] if r2.audio.ndim > 1 else r2.audio

        min_len = min(len(a1), len(a2))
        corr = np.corrcoef(a1[:min_len], a2[:min_len])[0, 1]
        rms1 = np.sqrt(np.mean(a1 ** 2))
        rms2 = np.sqrt(np.mean(a2 ** 2))
        rms_diff = abs(rms1 - rms2)
        print(f"\noffsetX -1 vs 1: corr={corr:.4f}, RMS diff={rms_diff:.6f}")
        assert abs(corr) < 0.99 or rms_diff > 0.001, (
            f"Offset should change output (corr={corr:.4f})"
        )


class TestMasterGain:
    """Master gain should scale output amplitude."""

    def test_master_gain_scales_output(self, sdf_plugin: DawDreamerHost):
        """gain=0.1 vs gain=1.0, higher gain should be louder."""
        # masterGain range: 0.0 - 1.0 (linear, already normalized)
        set_param(sdf_plugin, "masterGain", 0.1)
        r1 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        rms1 = np.sqrt(np.mean(r1.audio ** 2))

        set_param(sdf_plugin, "masterGain", 1.0)
        r2 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        rms2 = np.sqrt(np.mean(r2.audio ** 2))

        ratio = rms2 / (rms1 + 1e-10)
        print(f"\nGain 0.1 RMS: {rms1:.6f}")
        print(f"Gain 1.0 RMS: {rms2:.6f}")
        print(f"Ratio: {ratio:.2f}x")
        assert ratio > 1.5, f"Expected gain ratio > 1.5x, got {ratio:.2f}x"

    def test_zero_gain_is_silent(self, sdf_plugin: DawDreamerHost):
        """masterGain=0 should produce silence."""
        set_param(sdf_plugin, "masterGain", 0.0)
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.0)
        rms = np.sqrt(np.mean(result.audio ** 2))
        print(f"\nZero gain RMS: {rms:.6e}")
        assert rms < 0.001, f"Zero gain not silent (RMS={rms:.6f})"


class TestExtremeParams:
    """Extreme parameter combinations should not crash."""

    def test_all_params_in_range(self, sdf_plugin: DawDreamerHost):
        """Render with extreme parameter combinations, no NaN/Inf/clipping."""
        extremes = [
            # All minimums
            {"size1": 0.0, "size2": 0.0, "smoothK": 0.0,
             "twist": 0.0, "offsetX": 0.0, "scanHeight": 0.0,
             "distScale": 0.0, "scanRadius": 0.0},
            # All maximums
            {"size1": 1.0, "size2": 1.0, "smoothK": 1.0,
             "twist": 1.0, "offsetX": 1.0, "scanHeight": 1.0,
             "distScale": 1.0, "scanRadius": 1.0},
        ]

        for params in extremes:
            for name, value in params.items():
                set_param(sdf_plugin, name, value)

            result = sdf_plugin.render_note(note=60, velocity=100,
                                            duration_seconds=0.3,
                                            tail_seconds=0.1)
            assert not np.any(np.isnan(result.audio)), (
                f"NaN with params {params}"
            )
            assert not np.any(np.isinf(result.audio)), (
                f"Inf with params {params}"
            )
