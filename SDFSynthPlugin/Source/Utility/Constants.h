#pragma once

namespace sdf {

constexpr int TABLE_SIZE    = 2048;
constexpr int MAX_VOICES    = 16;
constexpr int MAX_UNISON    = 8;
constexpr int CONTOUR_RES   = 512;
constexpr int VOXEL_RES     = 24;
constexpr int CONTOUR_STEPS = 64;
constexpr int BINARY_ITERS  = 10;  // Reduced from 16: Illinois method converges faster
constexpr float MAX_RADIUS  = 2.0f;
constexpr float PI          = 3.14159265358979323846f;
constexpr float TWO_PI      = 6.28318530717958647692f;

constexpr int GRAIN_MAX_COUNT    = 256;
constexpr int GRAIN_SEARCH_STEPS = 32;
constexpr int GRAIN_BINARY_ITERS = 8;
constexpr float GOLDEN_ANGLE     = 2.39996323f;

constexpr int MARCH_SONIFY_MAX_RAYS   = 8;
constexpr int ACOUSTIC_NUM_RAYS       = 64;
constexpr int ACOUSTIC_MAX_BOUNCES    = 6;
constexpr int ACOUSTIC_MARCH_STEPS    = 48;
constexpr float GRADIENT_EPS          = 0.001f;
constexpr int SPECTRO_ANGLE_SAMPLES   = 256;
constexpr int SPECTRO_HEIGHT_SLICES   = 32;
constexpr int FIELD_PATH_OVERSAMPLE   = 4;

// Acoustic ray tracing
constexpr float ACOUSTIC_ENERGY_DECAY  = 0.6f;   // per-bounce energy retention
constexpr float ACOUSTIC_SURFACE_THRESH = 0.002f; // SDF threshold for surface hit
constexpr float ACOUSTIC_MIN_STEP      = 0.005f;  // minimum march step size
constexpr float ACOUSTIC_MAX_PATH      = 6.f;     // max total ray path length
constexpr float ACOUSTIC_PUSH_OFFSET   = 0.01f;   // surface push-off after bounce
constexpr float ACOUSTIC_ORIGIN_THRESH = 0.01f;   // SDF threshold for origin escape
constexpr int   ACOUSTIC_HANN_HALF     = 24;      // half-width of Hann pulse deposit
constexpr float ACOUSTIC_INTEGRATOR    = 0.995f;  // leaky integrator coefficient

// Granular curvature
constexpr float GRAIN_CURVATURE_EPS    = 0.005f;  // epsilon for Laplacian curvature
constexpr float GRAIN_MIN_FREQ         = 1.f;
constexpr float GRAIN_MAX_FREQ         = 80.f;
constexpr float GRAIN_CURVATURE_SCALE  = 8.f;     // curvature → frequency multiplier
constexpr int   GRAIN_MIN_COUNT        = 16;

// Volumetric spectrogram
constexpr float SPECTRO_HEIGHT_RANGE   = 0.4f;    // ±range from scanHeight

// Field traversal
constexpr int   FIELD_CROSSFADE_LEN    = 32;      // loop-point crossfade samples

// Ray march sonification
constexpr float MARCH_TUKEY_TAPER      = 0.15f;   // Tukey window taper fraction

// Mip-mapped wavetables
constexpr int MIP_LEVELS = 11;

// Modulation matrix
constexpr int MAX_MOD_SLOTS = 32;

} // namespace sdf

// Mod source colors (for GUI display) — harmonized with main accent palette
namespace sdfColour {
    constexpr unsigned int modEnvelope   = 0xff00ff88; // tertiaryAccent (green)
    constexpr unsigned int modLFO1       = 0xff00ffff; // primaryAccent (cyan)
    constexpr unsigned int modLFO2       = 0xffff6432; // secondaryAccent (orange)
    constexpr unsigned int modModWheel   = 0xff44bbcc; // muted cyan
    constexpr unsigned int modVelocity   = 0xffcc8844; // warm amber
    constexpr unsigned int modAftertouch = 0xff44cc99; // muted teal
    constexpr unsigned int modKeyTrack   = 0xff5599aa; // steel blue
    constexpr unsigned int modRandom     = 0xff778899; // steel gray (= mutedText)
    constexpr unsigned int modMacro      = 0xff66ccaa; // soft teal
}
