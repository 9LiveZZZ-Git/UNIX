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

// MipMappedWavetable is defined in SDFOscillator.h
struct MipMappedWavetable;

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
        float texScale = 1.5f);

    static MipMappedWavetable generateMipMap(const Wavetable& base);

private:
    static void sdfGradient(const SDFScene3D& scene, float x, float y, float z,
                            float& nx, float& ny, float& nz);
    static float findSurfaceRadius(const SDFScene3D& scene,
        float theta, float scanHeight);
    static Wavetable generateRayMarchSonify(const SDFScene3D& scene,
        float scanRadius, float scanHeight, float topoMorph, float distScale);
    static Wavetable generateAcousticTrace(const SDFScene3D& scene,
        float scanRadius, float scanHeight, float topoMorph, float distScale);
    static Wavetable generateGranularCurvature(const SDFScene3D& scene,
        float scanRadius, float scanHeight, float topoMorph, float distScale);
    static Wavetable generateVolumetricSpectro(const SDFScene3D& scene,
        float scanRadius, float scanHeight, float topoMorph, float distScale);
    static Wavetable generateFieldTraverse(const SDFScene3D& scene,
        float scanRadius, float scanHeight, float topoMorph, float distScale);

    // Simple radix-2 FFT helpers
    static void fft(std::vector<std::complex<float>>& data, bool inverse);
};
