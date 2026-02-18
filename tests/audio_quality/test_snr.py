"""
Signal-to-Noise Ratio (SNR) Tests

Measures the signal quality and noise floor of the plugin's output.

Synth-agnostic: works with any instrument plugin via DawDreamer.
"""

import numpy as np

import pytest

from .analyzers.audio_analyzer import AudioAnalyzer, SNRResult
from tests.tools.dawdreamer_host import DawDreamerHost, MIDIEvent


class TestSNR:
    """Signal-to-Noise Ratio tests using DawDreamer."""

    @pytest.fixture
    def analyzer(self, sample_rate):
        return AudioAnalyzer(sample_rate)

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_snr_active_vs_silence(self, analyzer, loaded_plugin: DawDreamerHost, thresholds):
        """
        Measure SNR by comparing active note to silence after note-off.

        The active signal should be much louder than the tail silence.
        """
        result = loaded_plugin.render_note(
            note=57,  # A220
            velocity=100,
            duration_seconds=1.0,
            tail_seconds=3.0
        )

        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        # Signal: during active note
        signal_start = int(0.1 * result.sample_rate)
        signal_end = int(0.8 * result.sample_rate)
        signal_power = np.mean(audio[signal_start:signal_end] ** 2)

        # Noise: well after note-off
        noise_start = int(2.5 * result.sample_rate)
        noise_end = int(3.5 * result.sample_rate)
        noise_power = np.mean(audio[noise_start:noise_end] ** 2)

        if noise_power > 0:
            snr_db = 10 * np.log10(signal_power / noise_power)
        else:
            snr_db = 120  # Effectively infinite

        print(f"\nSNR (active vs silence):")
        print(f"  Signal power: {signal_power:.6f}")
        print(f"  Noise power: {noise_power:.10f}")
        print(f"  SNR: {snr_db:.1f} dB")

        assert snr_db > 40, \
            f"SNR {snr_db:.1f} dB below threshold 40 dB"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_noise_floor(self, analyzer, loaded_plugin: DawDreamerHost):
        """
        Measure noise floor when plugin is idle.

        After note decay, output should approach digital silence.
        """
        result = loaded_plugin.render_note(
            note=60,
            velocity=100,
            duration_seconds=0.5,
            tail_seconds=5.0
        )

        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        # Measure at the end when note has decayed
        end_samples = int(0.5 * result.sample_rate)
        tail = audio[-end_samples:]

        rms = analyzer.measure_rms(tail)
        rms_db = 20 * np.log10(rms + 1e-10)

        print(f"\nNoise floor after decay:")
        print(f"  RMS: {rms:.2e} ({rms_db:.1f} dB)")

        # Should be very quiet after decay
        assert rms_db < -60, f"Noise floor {rms_db:.1f} dB too high (expected < -60 dB)"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_snr_vs_velocity(self, analyzer, loaded_plugin: DawDreamerHost):
        """
        Test SNR at different velocity levels.

        Lower velocities should still maintain acceptable SNR.
        Higher velocity should produce louder output.
        """
        velocity_values = [32, 64, 96, 127]
        results = []

        for velocity in velocity_values:
            result = loaded_plugin.render_note(
                note=57,  # A220
                velocity=velocity,
                duration_seconds=0.5,
                tail_seconds=1.0
            )

            audio = result.audio[0] if result.audio.ndim > 1 else result.audio

            start_sample = int(0.1 * result.sample_rate)
            end_sample = int(0.5 * result.sample_rate)

            if end_sample > len(audio):
                end_sample = len(audio)

            steady_state = audio[start_sample:end_sample]

            # Measure SNR using the analyzer
            snr_result = analyzer.measure_snr(steady_state, signal_freq=220)

            # Also measure RMS to track signal level
            rms = analyzer.measure_rms(steady_state)
            rms_db = 20 * np.log10(rms + 1e-10)

            results.append({
                'velocity': velocity,
                'snr_db': snr_result.snr_db,
                'noise_floor_db': snr_result.noise_floor_db,
                'rms_db': rms_db
            })

            print(f"\nVelocity {velocity}: SNR = {snr_result.snr_db:.1f} dB, "
                  f"RMS = {rms_db:.1f} dB, Noise floor = {snr_result.noise_floor_db:.1f} dB")

        # Verify velocity scaling: loudest should be louder than quietest
        rms_values = [r['rms_db'] for r in results]
        rms_range = max(rms_values) - min(rms_values)
        assert rms_range > 5, f"Expected >5dB dynamic range, got {rms_range:.1f}dB"

        # Verify rough monotonicity: vel 127 should be louder than vel 32
        assert results[-1]['rms_db'] > results[0]['rms_db'], \
            f"Velocity 127 ({results[-1]['rms_db']:.1f}dB) should be louder than velocity 32 ({results[0]['rms_db']:.1f}dB)"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_dynamic_range(self, analyzer, loaded_plugin: DawDreamerHost):
        """
        Measure effective dynamic range.

        From peak level to noise floor.
        """
        result = loaded_plugin.render_note(
            note=60,
            velocity=127,
            duration_seconds=0.5,
            tail_seconds=5.0
        )

        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        # Peak level (during active note)
        active = audio[:int(0.5 * result.sample_rate)]
        peak = analyzer.measure_peak(active)
        peak_db = 20 * np.log10(peak + 1e-10)

        # Noise floor (after decay)
        tail = audio[-int(0.5 * result.sample_rate):]
        noise_rms = analyzer.measure_rms(tail)
        noise_db = 20 * np.log10(noise_rms + 1e-10)

        dynamic_range = peak_db - noise_db

        print(f"\nDynamic range:")
        print(f"  Peak: {peak_db:.1f} dB")
        print(f"  Noise floor: {noise_db:.1f} dB")
        print(f"  Dynamic range: {dynamic_range:.1f} dB")

        # Should have good dynamic range
        assert dynamic_range > 40, \
            f"Dynamic range {dynamic_range:.1f} dB below expected 40 dB"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_no_clipping(self, loaded_plugin: DawDreamerHost):
        """
        Verify output doesn't clip.

        Check for samples at or near +-1.0.
        """
        result = loaded_plugin.render_note(
            note=60,
            velocity=127,
            duration_seconds=1.0,
            tail_seconds=0.5
        )

        audio = result.audio

        peak = np.max(np.abs(audio))
        num_clipped = np.sum(np.abs(audio) >= 0.999)

        print(f"\nClipping check:")
        print(f"  Peak: {peak:.4f}")
        print(f"  Samples at clip: {num_clipped}")

        assert peak < 1.0, f"Peak {peak:.4f} indicates clipping"
        assert num_clipped < 10, f"Found {num_clipped} clipped samples"
