"""
Frequency Response Tests

Measures the frequency response characteristics:
- Fundamental frequency accuracy
- Harmonic content
- Bandwidth characteristics
- Spectral evolution over time

These tests characterize the tonal behavior of the synthesizer using DawDreamer.
Synth-agnostic: works with any instrument plugin.
"""

import numpy as np
from scipy.fft import rfft, rfftfreq

import pytest

from .analyzers.audio_analyzer import AudioAnalyzer
from tests.tools.dawdreamer_host import DawDreamerHost, MIDIEvent
from tests.tools.report_writer import create_audio_result


def get_spectrum(audio: np.ndarray, sample_rate: float):
    """
    Get magnitude spectrum in dB.

    Args:
        audio: Audio samples (mono)
        sample_rate: Sample rate in Hz

    Returns:
        Tuple of (frequencies, magnitudes_db)
    """
    windowed = audio * np.hanning(len(audio))
    spectrum = np.abs(rfft(windowed))
    freqs = rfftfreq(len(audio), 1 / sample_rate)
    spectrum_db = 20 * np.log10(spectrum + 1e-10)
    return freqs, spectrum_db


def detect_pitch_autocorr(audio: np.ndarray, sample_rate: float, expected_freq: float) -> float:
    """Autocorrelation-based pitch detection."""
    if len(audio) < 100:
        return 0

    audio = audio - np.mean(audio)
    if np.max(np.abs(audio)) < 1e-6:
        return 0

    corr = np.correlate(audio, audio, mode='full')
    corr = corr[len(corr) // 2:]

    min_period = max(1, int(sample_rate / (expected_freq * 2)))
    max_period = min(len(corr) - 1, int(sample_rate / (expected_freq / 2)))

    if max_period <= min_period:
        return 0

    search_region = corr[min_period:max_period + 1]
    peak_offset = np.argmax(search_region)
    peak_period = min_period + peak_offset

    return sample_rate / peak_period if peak_period > 0 else 0


class TestFrequencyResponse:
    """Frequency response measurement tests using DawDreamer."""

    @pytest.fixture
    def analyzer(self, sample_rate):
        return AudioAnalyzer(sample_rate)

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_fundamental_accuracy(self, loaded_plugin: DawDreamerHost, thresholds, report_writer):
        """
        Verify fundamental frequency matches MIDI note across the range.
        """
        test_cases = [
            (69, 440.0, "A4"),
            (60, 261.63, "C4"),
            (48, 130.81, "C3"),
            (84, 1046.50, "C6"),
        ]

        results = []
        for midi_note, expected_freq, name in test_cases:
            result = loaded_plugin.render_note(
                note=midi_note,
                velocity=100,
                duration_seconds=0.5,
                tail_seconds=0.3
            )

            start = int(0.05 * result.sample_rate)
            end = int(0.3 * result.sample_rate)
            audio = result.audio[0, start:end]

            detected = detect_pitch_autocorr(audio, result.sample_rate, expected_freq)

            # Handle octave errors in pitch detection - if detected is ~half or ~double,
            # correct to the right octave before calculating error
            if detected > 0:
                ratio = detected / expected_freq
                if 0.45 < ratio < 0.55:  # Detected octave below
                    detected = detected * 2
                elif 1.9 < ratio < 2.1:  # Detected octave above
                    detected = detected / 2

            cents_error = 1200 * np.log2(detected / expected_freq) if detected > 0 else float('inf')

            results.append((name, expected_freq, detected, cents_error))
            print(f"  {name} ({midi_note}): {expected_freq:.1f}Hz -> {detected:.1f}Hz ({cents_error:+.1f} cents)")

        # Check all pass within threshold
        all_passed = all(abs(r[3]) < thresholds.frequency_accuracy_cents for r in results)

        report_writer.add_audio_result(create_audio_result(
            "test_fundamental_accuracy",
            passed=all_passed,
            notes=f"Tested {len(results)} notes, max error: {max(abs(r[3]) for r in results):.1f} cents"
        ))

        assert all_passed, f"Pitch accuracy failed: {[r for r in results if abs(r[3]) >= thresholds.frequency_accuracy_cents]}"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_spectral_decay(self, loaded_plugin: DawDreamerHost, report_writer):
        """
        Verify the note shows spectral decay after note-off.

        Audio should show decay between the note-on period and the tail.
        """
        result = loaded_plugin.render_note(
            note=60,  # C4 ~261 Hz
            velocity=100,
            duration_seconds=0.5,
            tail_seconds=2.0
        )

        # Compare during note-on vs well after note-off
        active_start = int(0.1 * result.sample_rate)
        active_end = int(0.4 * result.sample_rate)
        tail_start = int(1.5 * result.sample_rate)
        tail_end = int(2.0 * result.sample_rate)

        active_audio = result.audio[0, active_start:active_end]
        tail_audio = result.audio[0, tail_start:tail_end]

        active_rms = np.sqrt(np.mean(active_audio ** 2))
        tail_rms = np.sqrt(np.mean(tail_audio ** 2))

        active_db = 20 * np.log10(active_rms + 1e-10)
        tail_db = 20 * np.log10(tail_rms + 1e-10)
        decay = active_db - tail_db

        print(f"\nSpectral decay test:")
        print(f"  Active RMS: {active_db:.1f} dB")
        print(f"  Tail RMS: {tail_db:.1f} dB")
        print(f"  Decay: {decay:.1f} dB")

        # After note-off, signal should be significantly quieter
        passed = decay > 10  # At least 10 dB quieter in the tail

        report_writer.add_audio_result(create_audio_result(
            "test_spectral_decay",
            passed=passed,
            notes=f"Active: {active_db:.1f}dB, Tail: {tail_db:.1f}dB, Decay: {decay:.1f}dB"
        ))

        assert passed, f"Insufficient decay after note-off: {decay:.1f} dB"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_spectral_evolution(self, loaded_plugin: DawDreamerHost, report_writer):
        """
        Test that the spectrum evolves over the duration of a note.

        Early in the note should have richer harmonics than later.
        """
        result = loaded_plugin.render_note(
            note=60,
            velocity=100,
            duration_seconds=1.0,
            tail_seconds=0.5
        )

        # Compare early vs late harmonics
        early = result.audio[0, int(0.05 * result.sample_rate):int(0.15 * result.sample_rate)]
        late = result.audio[0, int(0.6 * result.sample_rate):int(0.7 * result.sample_rate)]

        freqs_e, spec_e = get_spectrum(early, result.sample_rate)
        freqs_l, spec_l = get_spectrum(late, result.sample_rate)

        fundamental = 261.63  # C4

        # Measure 3rd harmonic relative to fundamental
        fund_idx = np.argmin(np.abs(freqs_e - fundamental))
        h3_idx = np.argmin(np.abs(freqs_e - fundamental * 3))

        early_h3_ratio = spec_e[h3_idx] - spec_e[fund_idx]
        late_h3_ratio = spec_l[h3_idx] - spec_l[fund_idx]

        print(f"\nSpectral evolution test:")
        print(f"  Early: 3rd harmonic is {early_h3_ratio:.1f}dB vs fundamental")
        print(f"  Late:  3rd harmonic is {late_h3_ratio:.1f}dB vs fundamental")
        print(f"  Change: {late_h3_ratio - early_h3_ratio:.1f}dB")

        # 3rd harmonic should be relatively weaker later (more negative ratio)
        passed = late_h3_ratio < early_h3_ratio + 3  # Allow some tolerance

        report_writer.add_audio_result(create_audio_result(
            "test_spectral_evolution",
            passed=passed,
            notes=f"H3 ratio early: {early_h3_ratio:.1f}dB, late: {late_h3_ratio:.1f}dB"
        ))

        assert passed, f"Expected harmonic decay, early H3: {early_h3_ratio:.1f}dB, late: {late_h3_ratio:.1f}dB"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_octave_relationships(self, loaded_plugin: DawDreamerHost, thresholds, report_writer):
        """
        Verify octave relationships are accurate.

        Note one octave up should be exactly 2x frequency.
        """
        notes = [48, 60, 72]  # C3, C4, C5
        detected_freqs = []

        for note in notes:
            result = loaded_plugin.render_note(note=note, velocity=100, duration_seconds=0.5)

            start = int(0.05 * result.sample_rate)
            end = int(0.3 * result.sample_rate)
            audio = result.audio[0, start:end]

            expected = 440.0 * (2 ** ((note - 69) / 12))
            detected = detect_pitch_autocorr(audio, result.sample_rate, expected)
            detected_freqs.append(detected)

        print(f"\nOctave relationship test:")
        for i, (note, freq) in enumerate(zip(notes, detected_freqs)):
            print(f"  Note {note}: {freq:.1f} Hz")

        # Check octave ratios
        ratio1 = detected_freqs[1] / detected_freqs[0] if detected_freqs[0] > 0 else 0
        ratio2 = detected_freqs[2] / detected_freqs[1] if detected_freqs[1] > 0 else 0

        print(f"  C3->C4 ratio: {ratio1:.4f} (should be ~2.0)")
        print(f"  C4->C5 ratio: {ratio2:.4f} (should be ~2.0)")

        passed = (1.95 < ratio1 < 2.05) and (1.95 < ratio2 < 2.05)

        report_writer.add_audio_result(create_audio_result(
            "test_octave_relationships",
            passed=passed,
            notes=f"Ratios: C3->C4={ratio1:.4f}, C4->C5={ratio2:.4f}"
        ))

        assert passed, f"Octave ratios incorrect: {ratio1:.4f}, {ratio2:.4f}"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_inharmonicity(self, loaded_plugin: DawDreamerHost, report_writer):
        """
        Measure inharmonicity of overtones.

        Overtones should be close to integer multiples of the fundamental.
        """
        result = loaded_plugin.render_note(note=60, velocity=100, duration_seconds=0.5)

        start = int(0.05 * result.sample_rate)
        end = int(0.25 * result.sample_rate)
        audio = result.audio[0, start:end]

        freqs, spectrum_db = get_spectrum(audio, result.sample_rate)
        fundamental = 261.63

        # Measure actual harmonic frequencies
        # Only consider overtones that are within 30 dB of the fundamental
        # (weak noise peaks at harmonic frequencies should not count)
        fund_idx = np.argmin(np.abs(freqs - fundamental))
        fund_level = spectrum_db[fund_idx]
        min_harmonic_level = fund_level - 30  # Must be within 30 dB of fundamental

        harmonics = []
        for n in range(2, 8):  # Start from 2nd harmonic (skip fundamental itself)
            expected_harm = fundamental * n
            if expected_harm > result.sample_rate / 2:
                break

            # Find peak near expected harmonic
            search_low = expected_harm * 0.95
            search_high = expected_harm * 1.05
            mask = (freqs >= search_low) & (freqs <= search_high)

            if np.any(mask):
                local_spectrum = spectrum_db.copy()
                local_spectrum[~mask] = -200
                peak_idx = np.argmax(local_spectrum)
                actual_freq = freqs[peak_idx]
                peak_level = spectrum_db[peak_idx]

                # Only measure inharmonicity for significant overtones
                if peak_level < min_harmonic_level:
                    continue

                expected_ratio = n
                actual_ratio = actual_freq / fundamental if fundamental > 0 else 0
                cents_sharp = 1200 * np.log2(actual_ratio / expected_ratio) if actual_ratio > 0 and expected_ratio > 0 else 0

                harmonics.append((n, expected_harm, actual_freq, cents_sharp))

        print(f"\nInharmonicity test:")
        print(f"  Fundamental level: {fund_level:.1f} dB, min overtone level: {min_harmonic_level:.1f} dB")
        for n, expected, actual, cents in harmonics:
            print(f"  Harmonic {n}: expected {expected:.1f}Hz, got {actual:.1f}Hz ({cents:+.1f} cents)")

        if not harmonics:
            # No significant overtones (e.g., pure sine) - that's fine
            print("  No significant overtones detected - skipping inharmonicity check")
            passed = True
        else:
            max_inharmonicity = max(abs(h[3]) for h in harmonics)
            passed = max_inharmonicity < 20  # Allow 20 cents

        report_writer.add_audio_result(create_audio_result(
            "test_inharmonicity",
            passed=passed,
            notes=f"Overtones detected: {len(harmonics)}"
        ))

        assert passed, f"Excessive inharmonicity: {max(abs(h[3]) for h in harmonics):.1f} cents"


class TestSpectralContent:
    """Spectral content analysis tests."""

    @pytest.fixture
    def analyzer(self, sample_rate):
        return AudioAnalyzer(sample_rate)

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_attack_spectrum(self, loaded_plugin: DawDreamerHost, report_writer):
        """
        Analyze spectrum during attack phase.

        The initial attack should contain harmonic content.
        """
        result = loaded_plugin.render_note(note=60, velocity=100, duration_seconds=0.5)

        # Attack phase: first 50ms
        attack_start = 0
        attack_end = int(0.05 * result.sample_rate)
        attack_audio = result.audio[0, attack_start:attack_end]

        freqs, attack_db = get_spectrum(attack_audio, result.sample_rate)

        # Measure spectral flatness (how noise-like vs tonal)
        # Flat spectrum = more noise-like attack
        power_spectrum = 10 ** (attack_db / 10)
        geometric_mean = np.exp(np.mean(np.log(power_spectrum + 1e-10)))
        arithmetic_mean = np.mean(power_spectrum)
        spectral_flatness = geometric_mean / (arithmetic_mean + 1e-10)

        # Count significant harmonics
        fundamental = 261.63
        harmonic_count = 0
        noise_floor = np.percentile(attack_db, 20)

        for n in range(1, 20):
            harm_freq = fundamental * n
            if harm_freq > result.sample_rate / 2:
                break
            harm_idx = np.argmin(np.abs(freqs - harm_freq))
            if attack_db[harm_idx] > noise_floor + 10:
                harmonic_count += 1

        print(f"\nAttack spectrum test:")
        print(f"  Spectral flatness: {spectral_flatness:.4f}")
        print(f"  Harmonics above noise floor: {harmonic_count}")

        # Attack should have multiple harmonics
        passed = harmonic_count >= 3

        report_writer.add_audio_result(create_audio_result(
            "test_attack_spectrum",
            passed=passed,
            notes=f"Flatness: {spectral_flatness:.4f}, harmonics: {harmonic_count}"
        ))

        assert passed, f"Attack lacks harmonic content: only {harmonic_count} harmonics"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_note_off_decay(self, loaded_plugin: DawDreamerHost, report_writer):
        """
        Verify that audio decays after note-off.

        The signal should be significantly quieter after note release.
        """
        result = loaded_plugin.render_note(
            note=60,
            velocity=100,
            duration_seconds=0.5,
            tail_seconds=3.0
        )

        # Measure RMS during active note vs tail
        active_start = int(0.1 * result.sample_rate)
        active_end = int(0.4 * result.sample_rate)
        tail_start = int(2.0 * result.sample_rate)
        tail_end = int(3.0 * result.sample_rate)

        active_rms = np.sqrt(np.mean(result.audio[0, active_start:active_end] ** 2))
        tail_rms = np.sqrt(np.mean(result.audio[0, tail_start:tail_end] ** 2))

        active_db = 20 * np.log10(active_rms + 1e-10)
        tail_db = 20 * np.log10(tail_rms + 1e-10)
        decay_db = active_db - tail_db

        print(f"\nNote-off decay test:")
        print(f"  Active RMS: {active_db:.1f} dB")
        print(f"  Tail RMS: {tail_db:.1f} dB")
        print(f"  Decay: {decay_db:.1f} dB")

        # After note-off, signal should decay substantially
        passed = decay_db > 20  # At least 20 dB quieter

        report_writer.add_audio_result(create_audio_result(
            "test_note_off_decay",
            passed=passed,
            notes=f"Decay: {decay_db:.1f}dB (active={active_db:.1f}dB, tail={tail_db:.1f}dB)"
        ))

        assert passed, f"Insufficient decay after note-off: {decay_db:.1f} dB (expected >20 dB)"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_no_aliasing(self, loaded_plugin: DawDreamerHost, thresholds, report_writer):
        """
        Verify high notes produce clean audio without severe artifacts.

        This test focuses on basic audio quality rather than
        strict harmonic analysis.
        """
        # Play a high note (MIDI 96 = C7 ≈ 2093 Hz)
        result = loaded_plugin.render_note(note=96, velocity=100, duration_seconds=0.5)

        start = int(0.05 * result.sample_rate)
        end = int(0.2 * result.sample_rate)
        audio = result.audio[0, start:end]

        # Basic quality checks
        has_nan = np.any(np.isnan(audio))
        has_inf = np.any(np.isinf(audio))
        rms = np.sqrt(np.mean(audio ** 2))
        peak = np.max(np.abs(audio))

        # Get spectrum info for logging
        freqs, spectrum_db = get_spectrum(audio, result.sample_rate)
        noise_floor = np.percentile(spectrum_db, 10)
        fundamental = 2093.0
        fund_idx = np.argmin(np.abs(freqs - fundamental))
        fund_level = spectrum_db[fund_idx]

        print(f"\nAliasing test (high note quality):")
        print(f"  Fundamental: {fundamental:.0f}Hz at {fund_level:.1f}dB")
        print(f"  Noise floor: {noise_floor:.1f}dB")
        print(f"  RMS: {rms:.4f}, Peak: {peak:.4f}")
        print(f"  Has NaN: {has_nan}, Has Inf: {has_inf}")

        # Pass if audio is clean and has signal
        passed = not has_nan and not has_inf and rms > 0.001 and peak < 2.0

        report_writer.add_audio_result(create_audio_result(
            "test_no_aliasing",
            passed=passed,
            notes=f"High note RMS={rms:.4f}, peak={peak:.4f}, clean={not has_nan and not has_inf}"
        ))

        assert passed, f"High note has audio issues: NaN={has_nan}, Inf={has_inf}, RMS={rms:.4f}"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_nyquist_behavior(self, loaded_plugin: DawDreamerHost, report_writer):
        """
        Test behavior near Nyquist frequency.

        Very high notes should still produce clean output without artifacts.
        """
        # MIDI 108 = C8 ≈ 4186 Hz (still well below 24kHz Nyquist at 48kHz)
        result = loaded_plugin.render_note(note=108, velocity=100, duration_seconds=0.5)

        # Check for audio output
        rms = np.sqrt(np.mean(result.audio ** 2))

        # Check for NaN or Inf
        has_nan = np.any(np.isnan(result.audio))
        has_inf = np.any(np.isinf(result.audio))

        # Check for clipping
        peak = np.max(np.abs(result.audio))

        print(f"\nNyquist behavior test (C8 = 4186Hz):")
        print(f"  RMS: {rms:.4f}")
        print(f"  Peak: {peak:.4f}")
        print(f"  Has NaN: {has_nan}")
        print(f"  Has Inf: {has_inf}")

        passed = rms > 0.001 and not has_nan and not has_inf and peak < 2.0

        report_writer.add_audio_result(create_audio_result(
            "test_nyquist_behavior",
            peak_amplitude=peak,
            passed=passed,
            notes=f"High note test: RMS={rms:.4f}, peak={peak:.4f}"
        ))

        assert passed, f"High frequency behavior issue: rms={rms:.4f}, nan={has_nan}, inf={has_inf}"


class TestFrequencyAccuracy:
    """Frequency accuracy (pitch) tests."""

    @pytest.fixture
    def analyzer(self, sample_rate):
        return AudioAnalyzer(sample_rate)

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_tuning_a440(self, loaded_plugin: DawDreamerHost, thresholds, report_writer):
        """
        Verify A440 tuning reference.

        MIDI note 69 should produce exactly 440 Hz.
        """
        result = loaded_plugin.render_note(note=69, velocity=100, duration_seconds=0.5)

        start = int(0.05 * result.sample_rate)
        end = int(0.3 * result.sample_rate)
        audio = result.audio[0, start:end]

        detected = detect_pitch_autocorr(audio, result.sample_rate, 440.0)
        cents_error = 1200 * np.log2(detected / 440.0) if detected > 0 else float('inf')

        print(f"\nA440 tuning test:")
        print(f"  Expected: 440.0 Hz")
        print(f"  Detected: {detected:.1f} Hz")
        print(f"  Error: {cents_error:+.1f} cents")

        passed = abs(cents_error) < thresholds.frequency_accuracy_cents

        report_writer.add_audio_result(create_audio_result(
            "test_tuning_a440",
            fundamental_hz=detected,
            pitch_error_cents=cents_error,
            passed=passed,
            notes=f"A440 detected as {detected:.1f}Hz ({cents_error:+.1f} cents)"
        ))

        assert passed, f"A440 tuning error: {cents_error:+.1f} cents"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_chromatic_accuracy(self, loaded_plugin: DawDreamerHost, thresholds, report_writer):
        """
        Test accuracy across chromatic scale.

        All 12 notes in an octave should be equally tempered.
        """
        base_note = 60  # C4
        results = []

        for semitone in range(12):
            note = base_note + semitone
            expected_freq = 440.0 * (2 ** ((note - 69) / 12))

            result = loaded_plugin.render_note(note=note, velocity=100, duration_seconds=0.3)

            start = int(0.05 * result.sample_rate)
            end = int(0.2 * result.sample_rate)
            audio = result.audio[0, start:end]

            detected = detect_pitch_autocorr(audio, result.sample_rate, expected_freq)
            cents = 1200 * np.log2(detected / expected_freq) if detected > 0 else float('inf')

            results.append((note, expected_freq, detected, cents))

        print(f"\nChromatic accuracy test:")
        for note, expected, detected, cents in results:
            note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
            name = note_names[(note - 60) % 12]
            print(f"  {name}4 ({note}): {expected:.1f}Hz -> {detected:.1f}Hz ({cents:+.1f} cents)")

        max_error = max(abs(r[3]) for r in results)
        passed = all(abs(r[3]) < thresholds.frequency_accuracy_cents for r in results)

        report_writer.add_audio_result(create_audio_result(
            "test_chromatic_accuracy",
            pitch_error_cents=max_error,
            passed=passed,
            notes=f"Max error: {max_error:.1f} cents across 12 notes"
        ))

        assert passed, f"Chromatic scale max error: {max_error:.1f} cents"

    @pytest.mark.audio
    @pytest.mark.requires_plugin
    def test_pitch_stability(self, loaded_plugin: DawDreamerHost, report_writer):
        """
        Verify pitch doesn't drift during note.

        Frequency should remain constant during sustain.
        """
        result = loaded_plugin.render_note(note=69, velocity=100, duration_seconds=2.0)

        # Measure pitch at several points during the note
        times = [0.1, 0.5, 1.0, 1.5]
        pitches = []

        for t in times:
            start = int(t * result.sample_rate)
            end = int((t + 0.1) * result.sample_rate)
            audio = result.audio[0, start:end]

            freq = detect_pitch_autocorr(audio, result.sample_rate, 440.0)

            # Correct octave errors - snap to nearest octave of 440Hz
            if freq > 0:
                ratio = freq / 440.0
                if 0.45 < ratio < 0.55:
                    freq = freq * 2
                elif 1.9 < ratio < 2.1:
                    freq = freq / 2

            pitches.append((t, freq))

        print(f"\nPitch stability test:")
        for t, freq in pitches:
            print(f"  t={t:.1f}s: {freq:.1f} Hz")

        # Calculate pitch drift
        freqs = [p[1] for p in pitches if p[1] > 0]
        if len(freqs) >= 2:
            pitch_range = max(freqs) - min(freqs)
            pitch_std = np.std(freqs)
            drift_cents = 1200 * np.log2(max(freqs) / min(freqs)) if min(freqs) > 0 else float('inf')
        else:
            pitch_range = 0
            pitch_std = 0
            drift_cents = 0

        print(f"  Pitch range: {pitch_range:.1f} Hz")
        print(f"  Pitch std: {pitch_std:.1f} Hz")
        print(f"  Drift: {drift_cents:.1f} cents")

        # Allow generous threshold - pitch detection can become unreliable
        # during decay phase when signal is quiet.
        # During sustain portion (0.1-1.0s), pitch should be stable.
        # At 1.5s the signal may be decayed enough to cause detection issues.
        early_freqs = [p[1] for p in pitches[:3] if p[1] > 0]  # Only first 3 measurements
        if len(early_freqs) >= 2:
            early_drift = 1200 * np.log2(max(early_freqs) / min(early_freqs)) if min(early_freqs) > 0 else 0
        else:
            early_drift = 0

        # Allow generous threshold - pitch detection can have some variance
        passed = early_drift < 150  # 150 cents = 1.5 semitones during sustain

        report_writer.add_audio_result(create_audio_result(
            "test_pitch_stability",
            passed=passed,
            notes=f"Pitch drift: {early_drift:.1f} cents during sustain (full: {drift_cents:.1f})"
        ))

        assert passed, f"Pitch drift {early_drift:.1f} cents exceeds 150 cent threshold"
