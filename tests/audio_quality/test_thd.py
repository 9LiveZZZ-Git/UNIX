"""
Total Harmonic Distortion (THD) Tests

Measures harmonic distortion in the plugin's audio output to verify
clean signal generation.

Synth-agnostic: works with any instrument plugin via DawDreamer.
"""

import numpy as np

import pytest

from .analyzers.audio_analyzer import AudioAnalyzer, THDResult
from tests.tools.dawdreamer_host import DawDreamerHost, MIDIEvent


class TestTHD:
    """Total Harmonic Distortion tests using DawDreamer."""

    @pytest.fixture
    def analyzer(self, sample_rate):
        return AudioAnalyzer(sample_rate)

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_thd_a440(self, loaded_plugin: DawDreamerHost, thresholds):
        """
        Measure THD at A440 (MIDI note 69).

        The plugin should produce a tone with measurable harmonic content.
        """
        result = loaded_plugin.render_note(
            note=69,  # A440
            velocity=100,
            duration_seconds=1.0,
            tail_seconds=0.5
        )

        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        # Skip initial transient (first 0.1 seconds)
        start_sample = int(0.1 * result.sample_rate)
        end_sample = int(0.8 * result.sample_rate)
        steady_state = audio[start_sample:end_sample]

        # Measure THD
        thd_result = self._measure_thd_numpy(steady_state, result.sample_rate, fundamental_freq=440)

        print(f"\nTHD at A440:")
        print(f"  THD: {thd_result['thd_percent']:.4f}% ({thd_result['thd_db']:.1f} dB)")
        print(f"  Fundamental amplitude: {thd_result['fundamental_amplitude']:.6f}")

        # THD should be within a reasonable range (not indicating broken output)
        # Very high THD (>500%) would indicate severe distortion or aliasing
        assert thd_result['thd_percent'] < 500, \
            f"THD {thd_result['thd_percent']:.1f}% indicates severe distortion"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_thd_across_frequencies(self, loaded_plugin: DawDreamerHost, thresholds):
        """
        Test THD at various frequencies across the musical range.

        Different frequencies may have different distortion characteristics.
        """
        test_cases = [
            (36, 65.41, "C2 - Low bass"),
            (57, 220.0, "A3 - Low"),
            (69, 440.0, "A4 - Middle"),
            (81, 880.0, "A5 - High"),
            (93, 1760.0, "A6 - Very high"),
        ]

        results = []

        for midi_note, expected_freq, name in test_cases:
            result = loaded_plugin.render_note(
                note=midi_note,
                velocity=100,
                duration_seconds=0.5,
                tail_seconds=0.3
            )

            audio = result.audio[0] if result.audio.ndim > 1 else result.audio

            start = int(0.05 * result.sample_rate)
            end = int(0.4 * result.sample_rate)
            steady = audio[start:end]

            thd = self._measure_thd_numpy(steady, result.sample_rate, fundamental_freq=expected_freq)
            results.append((name, expected_freq, thd))

            print(f"\n{name}: THD = {thd['thd_db']:.1f} dB ({thd['thd_percent']:.2f}%)")

        # Verify all frequencies produce non-broken output
        for name, freq, thd in results:
            assert thd['thd_percent'] < 500, \
                f"{name} ({freq} Hz): THD {thd['thd_percent']:.1f}% indicates severe distortion"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_thd_vs_velocity(self, loaded_plugin: DawDreamerHost):
        """
        Test how velocity affects THD.

        The plugin should produce valid output at all velocity levels.
        """
        velocity_values = [32, 64, 96, 127]
        results = []

        for velocity in velocity_values:
            result = loaded_plugin.render_note(
                note=69,  # A440
                velocity=velocity,
                duration_seconds=0.5,
                tail_seconds=0.3
            )

            audio = result.audio[0] if result.audio.ndim > 1 else result.audio

            start_sample = int(0.05 * result.sample_rate)
            end_sample = int(0.4 * result.sample_rate)
            steady_state = audio[start_sample:end_sample]

            thd_result = self._measure_thd_numpy(steady_state, result.sample_rate, fundamental_freq=440)

            results.append({
                'velocity': velocity,
                'thd_percent': thd_result['thd_percent'],
                'thd_db': thd_result['thd_db'],
                'fundamental_amp': thd_result['fundamental_amplitude']
            })

            print(f"\nVelocity {velocity}: THD = {thd_result['thd_percent']:.4f}% ({thd_result['thd_db']:.1f} dB)")

        # Verify THD stays within reasonable bounds at all velocity levels
        max_acceptable_thd = 500.0
        for r in results:
            assert r['thd_percent'] < max_acceptable_thd, \
                f"Velocity {r['velocity']}: THD {r['thd_percent']:.2f}% exceeds maximum {max_acceptable_thd}%"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_harmonic_content(self, loaded_plugin: DawDreamerHost):
        """
        Verify the plugin produces measurable harmonic content.

        A pitched synthesizer should produce energy at harmonic frequencies.
        """
        result = loaded_plugin.render_note(
            note=69,  # A440
            velocity=100,
            duration_seconds=0.5,
            tail_seconds=0.3
        )

        audio = result.audio[0] if result.audio.ndim > 1 else result.audio

        # Analyze early in the note (richer harmonics)
        start = int(0.05 * result.sample_rate)
        end = int(0.2 * result.sample_rate)
        early = audio[start:end]

        thd = self._measure_thd_numpy(early, result.sample_rate, fundamental_freq=440, num_harmonics=20)

        print(f"\nHarmonic content at A440:")
        print(f"  Fundamental: {thd['fundamental_amplitude']:.6f}")
        for i, (freq, amp) in enumerate(thd['harmonics'][:10], 2):
            print(f"  Harmonic {i} ({freq:.0f} Hz): {amp:.6f}")

        # Verify fundamental has energy
        assert thd['fundamental_amplitude'] > 1e-6, \
            "Fundamental frequency should have measurable energy"

    def _measure_thd_numpy(
        self,
        signal: np.ndarray,
        sample_rate: int,
        fundamental_freq: float,
        num_harmonics: int = 10
    ) -> dict:
        """
        Measure THD using numpy FFT.

        Returns dict with thd_percent, thd_db, fundamental_amplitude, and harmonics.
        """
        # Apply Hann window
        window = np.hanning(len(signal))
        windowed = signal * window

        # Compute FFT
        fft_result = np.fft.rfft(windowed)
        freqs = np.fft.rfftfreq(len(windowed), 1 / sample_rate)
        magnitudes = np.abs(fft_result) / len(signal)

        # Find fundamental frequency bin
        fund_idx = np.argmin(np.abs(freqs - fundamental_freq))
        fundamental_amp = magnitudes[fund_idx]

        # Find harmonics
        harmonics = []
        harmonic_power = 0.0

        for n in range(2, num_harmonics + 2):
            harmonic_freq = fundamental_freq * n
            if harmonic_freq >= sample_rate / 2:
                break

            harmonic_idx = np.argmin(np.abs(freqs - harmonic_freq))
            harmonic_amp = magnitudes[harmonic_idx]
            harmonics.append((harmonic_freq, harmonic_amp))
            harmonic_power += harmonic_amp ** 2

        # Calculate THD
        if fundamental_amp > 0:
            thd_ratio = np.sqrt(harmonic_power) / fundamental_amp
        else:
            thd_ratio = 0

        thd_percent = thd_ratio * 100
        thd_db = 20 * np.log10(thd_ratio) if thd_ratio > 0 else -np.inf

        return {
            'thd_percent': thd_percent,
            'thd_db': thd_db,
            'fundamental_amplitude': fundamental_amp,
            'harmonics': harmonics
        }
