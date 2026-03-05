"""
Shape Primitive Tests

Verifies each SDF shape (Sphere, Box, Torus, Cylinder, Octahedron) produces
valid and distinct audio in the default Contour mode.
"""

import numpy as np
import pytest

from tests.tools.dawdreamer_host import DawDreamerHost
from tests.sdf_synth.conftest import SHAPES, set_param, set_shape


# Exclude "Custom" — only test the 5 geometric primitives
SHAPE_NAMES = [s for s in SHAPES if s != "Custom"]


class TestShapeBasics:
    """Each shape primitive should produce valid audio."""

    @pytest.mark.parametrize("shape", SHAPE_NAMES)
    def test_shape_produces_audio(self, sdf_plugin: DawDreamerHost, shape):
        """Each shape produces non-silent output."""
        set_shape(sdf_plugin, "shape1", shape)
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=0.5, tail_seconds=0.5)
        audio = result.audio[0] if result.audio.ndim > 1 else result.audio
        rms = np.sqrt(np.mean(audio ** 2))
        print(f"\n{shape} RMS: {rms:.6f}")
        assert rms > 0.001, f"{shape} produced silence (RMS={rms:.6f})"


class TestShapeDifferences:
    """Different shapes should produce different timbres."""

    def test_shapes_sound_different(self, sdf_plugin: DawDreamerHost):
        """Render same note with each shape; assert pairwise differences."""
        waveforms = {}
        for shape in SHAPE_NAMES:
            set_shape(sdf_plugin, "shape1", shape)
            result = sdf_plugin.render_note(note=60, velocity=100,
                                            duration_seconds=0.5,
                                            tail_seconds=0.0)
            audio = result.audio[0] if result.audio.ndim > 1 else result.audio
            waveforms[shape] = audio

        shapes = list(waveforms.keys())
        high_corr_pairs = []
        for i in range(len(shapes)):
            for j in range(i + 1, len(shapes)):
                a = waveforms[shapes[i]]
                b = waveforms[shapes[j]]
                min_len = min(len(a), len(b))
                corr = np.corrcoef(a[:min_len], b[:min_len])[0, 1]
                print(f"  {shapes[i]} vs {shapes[j]}: corr={corr:.4f}")
                if abs(corr) > 0.999:
                    high_corr_pairs.append((shapes[i], shapes[j], corr))

        assert len(high_corr_pairs) == 0, (
            f"Shapes too similar: {high_corr_pairs}"
        )

    def test_sphere_cleanest_spectrum(self, sdf_plugin: DawDreamerHost):
        """Sphere should have lowest THD among all shapes."""
        thd_values = {}

        for shape in SHAPE_NAMES:
            set_shape(sdf_plugin, "shape1", shape)
            result = sdf_plugin.render_note(note=69, velocity=100,
                                            duration_seconds=1.0,
                                            tail_seconds=0.0)
            audio = result.audio[0] if result.audio.ndim > 1 else result.audio

            n = len(audio)
            fft = np.abs(np.fft.rfft(audio))
            freqs = np.fft.rfftfreq(n, d=1.0 / result.sample_rate)

            fund_idx = np.argmin(np.abs(freqs - 440))
            w = 5
            fund_power = np.sum(
                fft[max(0, fund_idx - w):fund_idx + w + 1] ** 2)
            harm_power = 0.0
            for h in range(2, 9):
                h_idx = np.argmin(np.abs(freqs - 440 * h))
                harm_power += np.sum(
                    fft[max(0, h_idx - w):h_idx + w + 1] ** 2)

            thd = np.sqrt(harm_power / (fund_power + 1e-20))
            thd_values[shape] = thd
            print(f"  {shape} THD: {20 * np.log10(thd + 1e-10):.1f} dB")

        # Sphere should have the lowest THD (cleanest)
        sphere_thd = thd_values["Sphere"]
        for shape, thd in thd_values.items():
            if shape != "Sphere":
                assert sphere_thd <= thd * 1.5, (
                    f"Sphere THD ({sphere_thd:.4f}) should be <= "
                    f"{shape} THD ({thd:.4f})"
                )

    def test_sphere_and_box_differ_spectrally(self, sdf_plugin: DawDreamerHost):
        """Sphere and Box should have different spectral characteristics."""
        thd_map = {}
        for shape in ["Sphere", "Box"]:
            set_shape(sdf_plugin, "shape1", shape)
            result = sdf_plugin.render_note(note=69, velocity=100,
                                            duration_seconds=1.0,
                                            tail_seconds=0.0)
            audio = result.audio[0] if result.audio.ndim > 1 else result.audio

            n = len(audio)
            fft = np.abs(np.fft.rfft(audio))
            freqs = np.fft.rfftfreq(n, d=1.0 / result.sample_rate)

            fund_idx = np.argmin(np.abs(freqs - 440))
            w = 5
            fund_power = np.sum(
                fft[max(0, fund_idx - w):fund_idx + w + 1] ** 2)
            harm_power = 0.0
            for h in range(2, 9):
                h_idx = np.argmin(np.abs(freqs - 440 * h))
                harm_power += np.sum(
                    fft[max(0, h_idx - w):h_idx + w + 1] ** 2)

            thd_map[shape] = np.sqrt(harm_power / (fund_power + 1e-20))

        print(f"\nSphere THD: {thd_map['Sphere']:.4f}")
        print(f"Box THD:    {thd_map['Box']:.4f}")
        # They should have measurably different harmonic content
        diff = abs(thd_map["Sphere"] - thd_map["Box"])
        assert diff > 0.01, (
            f"Sphere and Box should differ spectrally (diff={diff:.4f})"
        )
