"""
Performance / CPU Benchmark Tests

Measures CPU usage for single and multi-voice rendering, scaling linearity,
and performance consistency.
"""

import numpy as np
import pytest

from tests.tools.dawdreamer_host import DawDreamerHost


class TestRealtimePerformance:
    """Core CPU performance checks."""

    @pytest.mark.performance
    def test_single_voice_realtime(self, sdf_plugin: DawDreamerHost):
        """Single voice renders faster than realtime."""
        result = sdf_plugin.render_note(note=60, velocity=100,
                                        duration_seconds=2.0, tail_seconds=0.0)
        cpu = result.realtime_ratio * 100
        print(f"\nSingle voice CPU: {cpu:.2f}%")
        assert result.realtime_ratio < 1.0, (
            f"Single voice not realtime ({result.realtime_ratio:.2f}x)"
        )

    @pytest.mark.performance
    def test_16_voices_cpu(self, sdf_plugin: DawDreamerHost):
        """16 voices CPU < 50% realtime."""
        result = sdf_plugin.render_polyphony_test(
            num_voices=16, base_note=36, velocity=100,
            stagger_ms=5, hold_seconds=2.0)

        cpu = result.realtime_ratio * 100
        print(f"\n16-voice CPU: {cpu:.2f}%")
        assert cpu < 50, f"16 voices used {cpu:.1f}% CPU (limit 50%)"


class TestCPUScaling:
    """CPU scaling with voice count."""

    @pytest.mark.performance
    def test_cpu_scaling(self, sdf_plugin: DawDreamerHost):
        """CPU should scale roughly linearly with voice count."""
        results = sdf_plugin.measure_cpu_per_voice(
            max_voices=16, note=60, duration=1.0)

        print(f"\nCPU scaling:")
        print(f"  {'Voices':>6} {'CPU %':>8} {'Per Voice':>10}")
        for voices, cpu in results:
            per_voice = cpu / voices
            print(f"  {voices:>6} {cpu:>7.1f}% {per_voice:>9.3f}%")

        per_voice_values = [cpu / v for v, cpu in results if v > 0]
        if len(per_voice_values) >= 2:
            ratio = per_voice_values[-1] / (per_voice_values[0] + 1e-10)
            assert ratio < 3.0, (
                f"CPU per voice increased {ratio:.1f}x (expected <3x)"
            )


class TestConsistency:
    """Performance should be stable across runs."""

    @pytest.mark.performance
    def test_consistent_performance(self, sdf_plugin: DawDreamerHost):
        """5 consecutive renders have low variance (CV < 0.5)."""
        cpu_samples = []

        for _ in range(5):
            result = sdf_plugin.render_note(note=60, velocity=100,
                                            duration_seconds=1.0,
                                            tail_seconds=0.0)
            cpu_samples.append(result.realtime_ratio * 100)

        mean_cpu = np.mean(cpu_samples)
        std_cpu = np.std(cpu_samples)
        cv = std_cpu / (mean_cpu + 1e-10)

        print(f"\nConsistency (5 renders):")
        print(f"  Mean CPU: {mean_cpu:.2f}%")
        print(f"  Std: {std_cpu:.3f}%")
        print(f"  CV: {cv:.3f}")
        assert cv < 0.5, f"Performance too variable (CV={cv:.3f})"
