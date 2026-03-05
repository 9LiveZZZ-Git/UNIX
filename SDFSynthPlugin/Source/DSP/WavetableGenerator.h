#pragma once
#include "SDFScene3D.h"
#include "ContourExtractor.h"
#include "ScanMode.h"
#include "Constants.h"
#include "Texture/TextureSampler.h"
#include "Texture/TextureSystem.h"
#include <array>
#include <vector>
#include <cmath>
#include <complex>
#include <memory>

// MipMappedWavetable is defined in SDFOscillator.h
struct MipMappedWavetable;

// Metadata exported from wavetable generation for accurate overlay visualization
struct ScanAlgorithmData
{
    // Traverse (Mode 5): exact Lissajous ratios from DSP
    float lissA = 1.f, lissB = 1.f, lissC = 0.f, lissDelta = 0.f;

    // RayMarch (Mode 1)
    int numRays = 1;
    float maxRange = 1.f;

    // Acoustic (Mode 2)
    int maxBounces = 1;

    // Granular (Mode 3)
    int numGrains = 16;
    struct GrainInfo { float angle; float freq; float curvature; };
    std::vector<GrainInfo> grainData;

    // Spectral (Mode 4)
    int activeBins = 8;
    std::array<float, sdf::SPECTRO_HEIGHT_SLICES> harmonicWeights{};
};

class WavetableGenerator
{
public:
    using Wavetable = std::array<float, sdf::TABLE_SIZE>;

    static Wavetable generate(const SDFScene3D& scene,
                               const std::vector<ContourPoint>& contour,
                               float scanRadius, float scanHeight,
                               float topoMorph, float distScale,
                               const TextureSlot* dispSlot = nullptr,
                               float dispAmt = 0.f,
                               const TextureSlot* emitSlot = nullptr,
                               float emIntensity = 0.f,
                               float texScale = 1.5f);

    static Wavetable generateWithMode(ScanMode mode,
        const SDFScene3D& scene,
        const std::vector<ContourPoint>& contour,
        float scanRadius, float scanHeight,
        float topoMorph, float distScale,
        const TextureSlot* dispSlot = nullptr, float dispAmt = 0.f,
        const TextureSlot* emitSlot = nullptr, float emIntensity = 0.f,
        float texScale = 1.5f,
        ScanAlgorithmData* outAlgData = nullptr);

    static MipMappedWavetable generateMipMap(const Wavetable& base);

private:
    static void sdfGradient(const SDFScene3D& scene, float x, float y, float z,
                            float& nx, float& ny, float& nz);
    static float findSurfaceRadius(const SDFScene3D& scene,
        float theta, float scanHeight);
    static Wavetable generateRayMarchSonify(const SDFScene3D& scene,
        float scanRadius, float scanHeight, float topoMorph, float distScale,
        ScanAlgorithmData* outAlgData = nullptr);
    static Wavetable generateAcousticTrace(const SDFScene3D& scene,
        float scanRadius, float scanHeight, float topoMorph, float distScale,
        ScanAlgorithmData* outAlgData = nullptr);
    static Wavetable generateGranularCurvature(const SDFScene3D& scene,
        float scanRadius, float scanHeight, float topoMorph, float distScale,
        ScanAlgorithmData* outAlgData = nullptr);
    static Wavetable generateVolumetricSpectro(const SDFScene3D& scene,
        float scanRadius, float scanHeight, float topoMorph, float distScale,
        ScanAlgorithmData* outAlgData = nullptr);
    static Wavetable generateFieldTraverse(const SDFScene3D& scene,
        float scanRadius, float scanHeight, float topoMorph, float distScale,
        ScanAlgorithmData* outAlgData = nullptr);

    // Simple radix-2 FFT helpers
    static void fft(std::vector<std::complex<float>>& data, bool inverse);
};
