"""
SDF Synth Plugin - Test Fixtures & Parameter Helpers

Provides SDF-Synth-specific fixtures on top of the shared test infrastructure.
Reuses DawDreamerHost, ThresholdConfig, and report_writer from tests/conftest.py.

NOTE: DawDreamer exposes JUCE parameters by their **display names**, not their
parameterID strings.  The PARAM_MAP below translates short IDs used in tests
to the display names DawDreamer reports.
"""

import platform
from pathlib import Path
from typing import Optional

import pytest

from tests.tools.dawdreamer_host import DawDreamerHost


# =============================================================================
# JUCE parameterID  ->  DawDreamer display name
# =============================================================================

PARAM_MAP = {
    # Shapes & operation
    "shape1":       "Shape A",
    "shape2":       "Shape B",
    "operation":    "Operation",
    # Scene
    "smoothK":      "Smooth K",
    "size1":        "Size A",
    "size2":        "Size B",
    "offsetX":      "Offset X",
    "offsetY":      "Offset Y",
    "twist":        "Twist",
    # Scan
    "scanRadius":   "Scan Radius",
    "scanHeight":   "Scan Height",
    "topoMorph":    "MRI Morph",
    "distScale":    "Distance Scale",
    "scanMode":     "Scan Mode",
    # ADSR
    "attack":       "Attack",
    "decay":        "Decay",
    "sustain":      "Sustain",
    "release":      "Release",
    # Filter
    "filterCutoff": "Filter Cutoff",
    "filterRes":    "Filter Resonance",
    # Master
    "masterGain":   "Master Volume",
}


# =============================================================================
# Parameter Constants
# =============================================================================

# AudioParameterChoice: normalized value = index / (N - 1)

SCAN_MODES = {
    "Contour":   0 / 5,  # 0.0
    "RayMarch":  1 / 5,  # 0.2
    "Acoustic":  2 / 5,  # 0.4
    "Granular":  3 / 5,  # 0.6
    "Spectral":  4 / 5,  # 0.8
    "Traverse":  5 / 5,  # 1.0
}

SHAPES = {
    "Sphere":     0 / 5,  # 0.0
    "Box":        1 / 5,  # 0.2
    "Torus":      2 / 5,  # 0.4
    "Cylinder":   3 / 5,  # 0.6
    "Octahedron": 4 / 5,  # 0.8
    "Custom":     5 / 5,  # 1.0
}

OPERATIONS = {
    "SmoothUnion":   0 / 3,  # 0.0
    "Union":         1 / 3,  # 0.333
    "Intersection":  2 / 3,  # 0.667
    "Subtraction":   3 / 3,  # 1.0
}


# =============================================================================
# Parameter Helpers
# =============================================================================

def _resolve(name: str) -> str:
    """Translate a short JUCE ID to its DawDreamer display name."""
    return PARAM_MAP.get(name, name)


def set_param(host: DawDreamerHost, name: str, value: float) -> bool:
    """Set a parameter using either a JUCE ID or a display name."""
    return host.set_parameter(_resolve(name), value)


def norm_float(value: float, min_val: float, max_val: float) -> float:
    """Convert a real parameter value to 0-1 normalized range (linear)."""
    return (value - min_val) / (max_val - min_val)


def set_scan_mode(host: DawDreamerHost, mode_name: str) -> bool:
    """Set the scan mode by name.  Returns False if the parameter is hidden."""
    return set_param(host, "scanMode", SCAN_MODES[mode_name])


def set_shape(host: DawDreamerHost, param: str, shape_name: str) -> bool:
    """Set shape1 or shape2 by name."""
    return set_param(host, param, SHAPES[shape_name])


def set_operation(host: DawDreamerHost, op_name: str) -> bool:
    """Set the boolean operation by name."""
    return set_param(host, "operation", OPERATIONS[op_name])


# =============================================================================
# Plugin Path Discovery
# =============================================================================

def _find_sdf_synth_vst3() -> Optional[Path]:
    """Search common build locations for the SDF Synth VST3."""
    from tests.conftest import get_project_root
    root = get_project_root()

    # Prefer Release over Debug builds
    for config in ["Release", "Debug", ""]:
        for base in [
            root / "SDFSynthPlugin" / "build",
            root / "plugin" / "SDFSynthPlugin" / "build",
        ]:
            if not base.exists():
                continue
            for vst3 in sorted(base.rglob("*.vst3")):
                # Prefer .vst3 bundles (directories), not the inner binary
                if not vst3.is_dir():
                    continue
                # Filter by config if specified
                if config and config not in str(vst3):
                    continue
                return vst3

    # Last resort: grab the first .vst3 anywhere under the build dirs
    for base in [
        root / "SDFSynthPlugin" / "build",
        root / "plugin" / "SDFSynthPlugin" / "build",
    ]:
        if base.exists():
            for vst3 in sorted(base.rglob("*.vst3")):
                return vst3

    return None


# =============================================================================
# Fixtures
# =============================================================================

@pytest.fixture(scope="session")
def sdf_plugin_path(request) -> Path:
    """
    Locate the SDF Synth VST3 plugin.

    Priority: --sdf-synth-path CLI > auto-discovery.
    Skips the entire session if not found.
    """
    override = request.config.getoption("--sdf-synth-path", default=None)
    if override:
        p = Path(override)
        if p.exists():
            return p
        pytest.skip(f"SDF Synth VST3 not found at: {override}")

    found = _find_sdf_synth_vst3()
    if found and found.exists():
        return found

    pytest.skip(
        "SDF Synth VST3 not found. Build it first or pass --sdf-synth-path."
    )


@pytest.fixture
def sdf_host(dawdreamer_available, sample_rate) -> DawDreamerHost:
    """Create a fresh DawDreamerHost for SDF Synth tests."""
    if not dawdreamer_available:
        pytest.skip("DawDreamer not installed. Run: pip install dawdreamer")
    host = DawDreamerHost(sample_rate=sample_rate)
    yield host
    host.unload()


@pytest.fixture
def sdf_plugin(sdf_host, sdf_plugin_path) -> DawDreamerHost:
    """
    DawDreamerHost with the SDF Synth VST3 loaded and ready.

    Function-scoped: each test gets a clean plugin instance.
    """
    if not sdf_host.load_plugin(sdf_plugin_path):
        pytest.skip(f"Failed to load SDF Synth: {sdf_plugin_path}")
    return sdf_host
