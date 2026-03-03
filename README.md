# SDF Synth Plugin

A 3D signed-distance-field wavetable synthesizer built with JUCE 8. Renders SDF geometry in real-time via OpenGL, extracts wavetables from contour/ray-march scans, and plays them through a full synthesis + FX pipeline.

## Build & Run

Requires CMake, Visual Studio 2019 (v16), and a GPU with OpenGL 3.3+.

```bash
cd SDFSynthPlugin
cmake -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Release
```

Run the standalone:
```
build/SDFSynthPlugin_artefacts/Release/Standalone/SDF Synth.exe
```

The VST3 is at `build/SDFSynthPlugin_artefacts/Release/VST3/SDF Synth.vst3`.

## Features

- **SDF viewport** -- real-time PBR rendering with Cook-Torrance BRDF, soft shadows, AO, ACES tone mapping
- **12 SDF primitives** (Sphere, Box, Torus, Octahedron, Capsule, HexPrism, SuperFormula, etc.) with 10 CSG operations
- **6 scan modes** for wavetable extraction (Contour, March, Acoustic, Grain, Spectral, Traverse)
- **Dual oscillator** with Add/Ring/FM/AM mixing, wavefold, phase distortion, hard sync
- **Unison** 1-8 voices with stereo spread and detune
- **Modulation matrix** -- 32 slots, 12 sources (2 LFOs, envelope, velocity, macros, etc.)
- **FX chain** -- Distortion, Chorus, Delay, Reverb
- **SVF filter** -- LP/BP/HP/Notch/Peak with per-sample smoothing
- **30 factory presets**, ~85 parameters, OBJ mesh import via voxelization

## Run Tests

```bash
cmake --build build --config Release --target SDFSynthTests
build/SDFSynthTests_artefacts/Release/SDFSynthTests.exe
```

---

# UNIX Audio Exercises

Build and test like this:

    mkdir build
    cd build
    cmake ..
    cmake --build .
    export PATH=$PATH:`pwd`
    sine-sweep | wav-write

UNIX philosophy is sortof like Max. We make lots of little programs that generate outputs and/or analyse or process inputs. We then wires these together to make more complex programs.

Complete empty code files:

- `impulse.cpp`
- `sawtooth.cpp`
- `square.cpp`
- `triangle.cpp`
- `impulse-sweep.cpp`
- `sawtooth-sweep.cpp`
- `square-sweep.cpp`
- `triangle-sweep.cpp`

and change `CMakeLists.txt` to add new executable targets.
