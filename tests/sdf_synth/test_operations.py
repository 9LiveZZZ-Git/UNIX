"""
Boolean Operation Tests

Verifies each SDF boolean operation (SmoothUnion, Union, Intersection,
Subtraction) produces valid and distinct audio.
"""

import numpy as np
import pytest

from tests.tools.dawdreamer_host import DawDreamerHost
from tests.sdf_synth.conftest import (
    OPERATIONS, set_param, set_shape, set_operation, norm_float,
)


OPERATION_NAMES = list(OPERATIONS.keys())


class TestOperationBasics:
    """Each boolean operation should produce valid audio."""

    @pytest.mark.parametrize("op", OPERATION_NAMES)
    def test_operation_produces_audio(self, sdf_plugin: DawDreamerHost, op):
        """Each operation produces non-silent output."""
        set_shape(sdf_plugin, "shape1", "Sphere")
        set_shape(sdf_plugin, "shape2", "Box")
        set_operation(sdf_plugin, op)

        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.5)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio
        rms = np.sqrt(np.mean(audio ** 2))
        print(f"\n{op} RMS: {rms:.6f}")
        assert rms > 0.001, f"{op} produced silence (RMS={rms:.6f})"


class TestOperationDifferences:
    """Different operations should produce different results."""

    def test_operations_sound_different(self, sdf_plugin: DawDreamerHost):
        """Different operations on same shapes yield different waveforms."""
        set_shape(sdf_plugin, "shape1", "Sphere")
        set_shape(sdf_plugin, "shape2", "Torus")

        waveforms = {}
        for op in OPERATION_NAMES:
            set_operation(sdf_plugin, op)
            result = sdf_plugin.render_note(note=60, velocity=100,
                                            duration_seconds=0.5,
                                            tail_seconds=0.0)
            audio = result.audio[0] if result.audio.ndim > 1 else result.audio
            waveforms[op] = audio

        ops = list(waveforms.keys())
        high_corr_pairs = []
        for i in range(len(ops)):
            for j in range(i + 1, len(ops)):
                a = waveforms[ops[i]]
                b = waveforms[ops[j]]
                min_len = min(len(a), len(b))
                corr = np.corrcoef(a[:min_len], b[:min_len])[0, 1]
                print(f"  {ops[i]} vs {ops[j]}: corr={corr:.4f}")
                if abs(corr) > 0.99:
                    high_corr_pairs.append((ops[i], ops[j], corr))

        assert len(high_corr_pairs) == 0, (
            f"Operations too similar: {high_corr_pairs}"
        )

    def test_smooth_k_affects_output(self, sdf_plugin: DawDreamerHost):
        """Changing smoothK from min to max changes the output waveform."""
        set_shape(sdf_plugin, "shape1", "Sphere")
        set_shape(sdf_plugin, "shape2", "Box")
        set_operation(sdf_plugin, "SmoothUnion")

        # smoothK range: 0.01 - 1.5
        set_param(sdf_plugin, "smoothK", norm_float(0.01, 0.01, 1.5))
        r1 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a1 = r1.audio[0] if r1.audio.ndim > 1 else r1.audio

        set_param(sdf_plugin, "smoothK", norm_float(1.5, 0.01, 1.5))
        r2 = sdf_plugin.render_note(note=60, velocity=100,
                                    duration_seconds=0.5, tail_seconds=0.0)
        a2 = r2.audio[0] if r2.audio.ndim > 1 else r2.audio

        min_len = min(len(a1), len(a2))
        corr = np.corrcoef(a1[:min_len], a2[:min_len])[0, 1]
        rms1 = np.sqrt(np.mean(a1 ** 2))
        rms2 = np.sqrt(np.mean(a2 ** 2))
        rms_diff = abs(rms1 - rms2)
        print(f"\nsmoothK min vs max: corr={corr:.4f}, RMS diff={rms_diff:.6f}")
        assert abs(corr) < 0.99 or rms_diff > 0.001, (
            f"smoothK should change output (corr={corr:.4f})"
        )
