# UNIX Audio

Audio DSP plugins built with JUCE 8 and C++17. Each plugin builds independently.

## Karplus-Strong Synth

Physical-modelling string synthesizer using Karplus-Strong algorithm.

### Build

```bash
cd KarplusStrongPlugin
cmake -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Release
```

### Run

```
build/KarplusStrongPlugin_artefacts/Release/Standalone/Karplus-Strong Synth.exe
```

VST3: `build/KarplusStrongPlugin_artefacts/Release/VST3/Karplus-Strong Synth.vst3`

### Tests

```bash
cmake -B build -G "Visual Studio 16 2019" -A x64 -DBUILD_TESTS=ON
cmake --build build --config Release --target KarplusStrongTests
build/KarplusStrongTests_artefacts/Release/KarplusStrongTests.exe
```

---

## SDF Synth

3D signed-distance-field wavetable synthesizer with real-time OpenGL viewport. Extracts wavetables from SDF geometry via contour/ray-march scans.

### Build

Requires OpenGL 3.3+.

```bash
cd SDFSynthPlugin
cmake -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Release
```

### Run

```
build/SDFSynthPlugin_artefacts/Release/Standalone/SDF Synth.exe
```

VST3: `build/SDFSynthPlugin_artefacts/Release/VST3/SDF Synth.vst3`

### Tests

```bash
cmake --build build --config Release --target SDFSynthTests
build/SDFSynthTests_artefacts/Release/SDFSynthTests.exe
```

### Features

- **SDF viewport** -- real-time PBR rendering (Cook-Torrance GGX, soft shadows, AO, ACES tone mapping)
- **12 SDF primitives** (Sphere, Box, Torus, Octahedron, Capsule, HexPrism, SuperFormula, etc.) with 10 CSG ops
- **6 scan modes** for wavetable extraction (Contour, March, Acoustic, Grain, Spectral, Traverse)
- **Dual oscillator** with Add/Ring/FM/AM mixing, wavefold, phase distortion, hard sync
- **Unison** 1-8 voices with stereo spread and detune
- **Modulation matrix** -- 32 slots, 12 sources (2 LFOs, envelope, velocity, macros, etc.)
- **FX chain** -- Distortion, Chorus, Delay, Reverb
- **SVF filter** -- LP/BP/HP/Notch/Peak with per-sample smoothing
- **30 factory presets**, ~85 parameters, OBJ mesh import

---

## UNIX Audio Exercises

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
