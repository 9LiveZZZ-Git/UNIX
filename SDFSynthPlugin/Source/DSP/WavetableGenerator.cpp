#include "WavetableGenerator.h"
#include "SDFOscillator.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <complex>

// Forward declarations of post-processing helpers (defined below)
static void removeDCAndNormalize(std::array<float, sdf::TABLE_SIZE>& table);
static void smoothTable(std::array<float, sdf::TABLE_SIZE>& table);
static void crossfadeLoopPoint(std::array<float, sdf::TABLE_SIZE>& table, int fadeLen);

WavetableGenerator::Wavetable WavetableGenerator::generate(
    const SDFScene3D& scene,
    const std::vector<ContourPoint>& contour,
    float scanRadius, float scanHeight,
    float topoMorph, float distScale,
    const TextureSlot* dispSlot, float dispAmt,
    const TextureSlot* emitSlot, float emIntensity,
    float texScale)
{
    Wavetable table{};
    const int n = sdf::TABLE_SIZE;

    if (topoMorph > 0.5f)
    {
        // MRI mode: waveform is the radial profile of the cross-section
        std::array<float, sdf::TABLE_SIZE> radii{};
        for (int i = 0; i < n; ++i)
        {
            float theta = (static_cast<float>(i) / n) * sdf::TWO_PI;
            auto pt = ContourExtractor::getScanPoint(theta, contour, scanRadius, topoMorph);
            radii[i] = pt.r;
        }

        float meanR = std::accumulate(radii.begin(), radii.end(), 0.f) / n;
        float maxDev = 0.01f;
        for (int i = 0; i < n; ++i)
            maxDev = std::max(maxDev, std::abs(radii[i] - meanR));

        float sdfMix = std::max(0.f, (1.f - topoMorph) * 2.f);

        for (int i = 0; i < n; ++i)
        {
            float profileVal = (radii[i] - meanR) / maxDev;
            float theta = (static_cast<float>(i) / n) * sdf::TWO_PI;
            auto pt = ContourExtractor::getScanPoint(theta, contour, scanRadius, topoMorph);
            float sdfVal = scene.evaluate(pt.x, scanHeight, pt.z) * distScale;
            float s = std::clamp(profileVal * (1.f - sdfMix * 0.5f) + sdfVal * sdfMix * 0.5f, -1.f, 1.f);

            if (dispSlot != nullptr && dispSlot->loaded && dispAmt > 0.001f)
            {
                float dv = TextureSampler::sampleTexCPU(*dispSlot, pt.x, scanHeight, pt.z, texScale);
                s += (dv - 0.5f) * dispAmt * 4.f;
            }

            if (emitSlot != nullptr && emitSlot->loaded && emIntensity > 0.001f)
            {
                float ev = TextureSampler::sampleTexCPU(*emitSlot, pt.x, scanHeight, pt.z, texScale);
                s *= 1.f + ev * emIntensity * 0.8f;
            }

            table[i] = std::clamp(s, -1.f, 1.f);
        }
    }
    else
    {
        for (int i = 0; i < n; ++i)
        {
            float theta = (static_cast<float>(i) / n) * sdf::TWO_PI;
            auto pt = ContourExtractor::getScanPoint(theta, contour, scanRadius, topoMorph);
            float d = scene.evaluate(pt.x, scanHeight, pt.z);
            float s = std::clamp(d * distScale, -1.f, 1.f);

            if (dispSlot != nullptr && dispSlot->loaded && dispAmt > 0.001f)
            {
                float dv = TextureSampler::sampleTexCPU(*dispSlot, pt.x, scanHeight, pt.z, texScale);
                s += (dv - 0.5f) * dispAmt * 4.f;
            }

            if (emitSlot != nullptr && emitSlot->loaded && emIntensity > 0.001f)
            {
                float ev = TextureSampler::sampleTexCPU(*emitSlot, pt.x, scanHeight, pt.z, texScale);
                s *= 1.f + ev * emIntensity * 0.8f;
            }

            table[i] = std::clamp(s, -1.f, 1.f);
        }
    }

    // Post-process: smooth SDF sampling noise, close loop point, normalize
    crossfadeLoopPoint(table, 32);
    smoothTable(table);
    removeDCAndNormalize(table);

    return table;
}

// ── Mode-aware dispatcher ──────────────────────────────────────────

WavetableGenerator::Wavetable WavetableGenerator::generateWithMode(
    ScanMode mode,
    const SDFScene3D& scene,
    const std::vector<ContourPoint>& contour,
    float scanRadius, float scanHeight,
    float topoMorph, float distScale,
    const TextureSlot* dispSlot, float dispAmt,
    const TextureSlot* emitSlot, float emIntensity,
    float texScale,
    ScanAlgorithmData* outAlgData)
{
    switch (mode)
    {
        case ScanMode::RayMarchSonify:
            return generateRayMarchSonify(scene, scanRadius, scanHeight, topoMorph, distScale, outAlgData);
        case ScanMode::AcousticTrace:
            return generateAcousticTrace(scene, scanRadius, scanHeight, topoMorph, distScale, outAlgData);
        case ScanMode::GranularCurvature:
            return generateGranularCurvature(scene, scanRadius, scanHeight, topoMorph, distScale, outAlgData);
        case ScanMode::VolumetricSpectro:
            return generateVolumetricSpectro(scene, scanRadius, scanHeight, topoMorph, distScale, outAlgData);
        case ScanMode::FieldTraverse:
            return generateFieldTraverse(scene, scanRadius, scanHeight, topoMorph, distScale, outAlgData);
        case ScanMode::Contour:
        default:
            return generate(scene, contour, scanRadius, scanHeight,
                            topoMorph, distScale, dispSlot, dispAmt,
                            emitSlot, emIntensity, texScale);
    }
}

// ── SDF Gradient helper (tetrahedron technique, 4 evals) ────────────

void WavetableGenerator::sdfGradient(const SDFScene3D& scene,
    float x, float y, float z,
    float& nx, float& ny, float& nz)
{
    constexpr float e = sdf::GRADIENT_EPS;
    // Tetrahedron vertices: (+,+,-), (-,-,-), (-,+,+), (+,-,+)
    float d0 = scene.evaluate(x + e, y + e, z - e);
    float d1 = scene.evaluate(x - e, y - e, z - e);
    float d2 = scene.evaluate(x - e, y + e, z + e);
    float d3 = scene.evaluate(x + e, y - e, z + e);
    nx = d0 - d1 - d2 + d3;
    ny = d0 - d1 + d2 - d3;
    nz = -d0 - d1 + d2 + d3;
    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 1e-8f) { nx /= len; ny /= len; nz /= len; }
}

// ── findSurfaceRadius ──────────────────────────────────────────────

float WavetableGenerator::findSurfaceRadius(
    const SDFScene3D& scene, float theta, float scanHeight)
{
    float dirX = std::cos(theta);
    float dirZ = std::sin(theta);

    float t = 0.f;
    float prevD = scene.evaluate(0.f, scanHeight, 0.f);
    float dt = sdf::MAX_RADIUS / sdf::GRAIN_SEARCH_STEPS;

    for (int s = 0; s < sdf::GRAIN_SEARCH_STEPS; ++s)
    {
        t += dt;
        float d = scene.evaluate(dirX * t, scanHeight, dirZ * t);
        if ((prevD > 0.f) != (d > 0.f))
        {
            float lo = t - dt, hi = t;
            for (int b = 0; b < sdf::GRAIN_BINARY_ITERS; ++b)
            {
                float mid = (lo + hi) * 0.5f;
                float dm = scene.evaluate(dirX * mid, scanHeight, dirZ * mid);
                if ((prevD > 0.f) != (dm > 0.f))
                    hi = mid;
                else
                    lo = mid;
            }
            return (lo + hi) * 0.5f;
        }
        prevD = d;
    }
    return sdf::MAX_RADIUS;
}

// ── Shared post-processing helpers ─────────────────────────────────

// Combined DC removal + normalize (saves iterations vs separate functions)
static void removeDCAndNormalize(std::array<float, sdf::TABLE_SIZE>& table)
{
    const int n = sdf::TABLE_SIZE;
    float dc = 0.f;
    float maxAbs = 0.01f;
    // Pass 1: compute DC and max abs in one loop
    for (int i = 0; i < n; ++i)
        dc += table[i];
    dc /= n;
    // Pass 2: subtract DC, find max, and normalize
    for (int i = 0; i < n; ++i)
    {
        table[i] -= dc;
        maxAbs = std::max(maxAbs, std::abs(table[i]));
    }
    float invMax = 1.f / maxAbs;
    for (int i = 0; i < n; ++i)
        table[i] = std::clamp(table[i] * invMax, -1.f, 1.f);
}

// 5-tap triangular smooth with wrapping — removes sharp discontinuities
// Uses explicit boundary handling instead of per-sample modulo
static void smoothTable(std::array<float, sdf::TABLE_SIZE>& table)
{
    constexpr int n = sdf::TABLE_SIZE;
    std::array<float, sdf::TABLE_SIZE> smoothed{};

    // Handle first 2 samples (wrap from end)
    smoothed[0] = table[n - 2] * 0.1f + table[n - 1] * 0.2f + table[0] * 0.4f + table[1] * 0.2f + table[2] * 0.1f;
    smoothed[1] = table[n - 1] * 0.1f + table[0] * 0.2f + table[1] * 0.4f + table[2] * 0.2f + table[3] * 0.1f;

    // Main body — no modulo needed
    for (int i = 2; i < n - 2; ++i)
    {
        smoothed[i] = table[i - 2] * 0.1f
                    + table[i - 1] * 0.2f
                    + table[i] * 0.4f
                    + table[i + 1] * 0.2f
                    + table[i + 2] * 0.1f;
    }

    // Handle last 2 samples (wrap to start)
    smoothed[n - 2] = table[n - 4] * 0.1f + table[n - 3] * 0.2f + table[n - 2] * 0.4f + table[n - 1] * 0.2f + table[0] * 0.1f;
    smoothed[n - 1] = table[n - 3] * 0.1f + table[n - 2] * 0.2f + table[n - 1] * 0.4f + table[0] * 0.2f + table[1] * 0.1f;

    table = smoothed;
}

// Crossfade first/last N samples to close loop point
static void crossfadeLoopPoint(std::array<float, sdf::TABLE_SIZE>& table, int fadeLen)
{
    const int n = sdf::TABLE_SIZE;
    fadeLen = std::min(fadeLen, n / 4);
    for (int i = 0; i < fadeLen; ++i)
    {
        float t = static_cast<float>(i) / fadeLen;
        float cosT = 0.5f * (1.f - std::cos(sdf::PI * t));
        int startIdx = i;
        int endIdx = n - fadeLen + i;
        float origStart = table[startIdx];
        float origEnd = table[endIdx];
        table[startIdx] = origEnd * (1.f - cosT) + origStart * cosT;
        table[endIdx]   = origEnd * cosT + origStart * (1.f - cosT);
    }
}

// ── Mode 1: Ray March Sonification ─────────────────────────────────
// FIX: Smooth boundary clicks between ray segments, add DC removal

WavetableGenerator::Wavetable WavetableGenerator::generateRayMarchSonify(
    const SDFScene3D& scene,
    float scanRadius, float scanHeight, float topoMorph, float distScale,
    ScanAlgorithmData* outAlgData)
{
    Wavetable table{};
    const int n = sdf::TABLE_SIZE;

    int numRays = 1 + static_cast<int>(std::floor(topoMorph * 7.f));
    numRays = std::clamp(numRays, 1, sdf::MARCH_SONIFY_MAX_RAYS);
    int samplesPerRay = n / numRays;
    float maxRange = scanRadius * 2.f;

    // Phase 4a: adaptive Tukey taper — wider crossfade for more rays
    float taper = sdf::MARCH_TUKEY_TAPER + 0.1f * (static_cast<float>(numRays) / 8.f);

    // Export algorithm data
    if (outAlgData)
    {
        outAlgData->numRays = numRays;
        outAlgData->maxRange = maxRange;
    }

    for (int ray = 0; ray < numRays; ++ray)
    {
        float azimuth = ray * sdf::GOLDEN_ANGLE;
        float dirX = std::cos(azimuth);
        float dirZ = std::sin(azimuth);

        for (int s = 0; s < samplesPerRay; ++s)
        {
            float frac = static_cast<float>(s) / samplesPerRay;
            float t = frac * maxRange;

            float px = dirX * t;
            float pz = dirZ * t;
            float d = scene.evaluate(px, scanHeight, pz);

            // tanh mapping: near misses -> sharp notches, interior -> negative
            float val = std::tanh(d * distScale);

            // Tukey window per segment: cosine taper first/last portion
            float window = 1.f;
            if (frac < taper)
                window = 0.5f * (1.f - std::cos(sdf::PI * frac / taper));
            else if (frac > (1.f - taper))
                window = 0.5f * (1.f - std::cos(sdf::PI * (1.f - frac) / taper));

            int idx = ray * samplesPerRay + s;
            if (idx < n)
                table[idx] = val * window;
        }
    }

    // Fill any remaining samples (from integer division)
    for (int i = numRays * samplesPerRay; i < n; ++i)
        table[i] = table[i % (numRays * samplesPerRay)];

    // Smooth boundaries between segments with wrapping triangular filter
    smoothTable(table);

    removeDCAndNormalize(table);

    return table;
}

// ── Mode 2: Acoustic Ray Tracing ───────────────────────────────────
// FIX: Push rays outside surface if starting inside, warm up integrator,
//      widen Hann pulses

WavetableGenerator::Wavetable WavetableGenerator::generateAcousticTrace(
    const SDFScene3D& scene,
    float /*scanRadius*/, float scanHeight, float topoMorph, float distScale,
    ScanAlgorithmData* outAlgData)
{
    Wavetable table{};
    const int n = sdf::TABLE_SIZE;

    int maxBounces = 1 + static_cast<int>(std::floor(topoMorph * 5.f));
    maxBounces = std::clamp(maxBounces, 1, sdf::ACOUSTIC_MAX_BOUNCES);

    // Export algorithm data
    if (outAlgData)
        outAlgData->maxBounces = maxBounces;

    for (int r = 0; r < sdf::ACOUSTIC_NUM_RAYS; ++r)
    {
        // Fibonacci sphere distribution
        float t = static_cast<float>(r) / sdf::ACOUSTIC_NUM_RAYS;
        float phi = std::acos(1.f - 2.f * t);
        float theta = sdf::GOLDEN_ANGLE * r;

        float dirX = std::sin(phi) * std::cos(theta);
        float dirY = std::sin(phi) * std::sin(theta);
        float dirZ = std::cos(phi);

        // Ray origin at mic position
        float ox = 0.f, oy = scanHeight, oz = 0.f;

        // FIX: If origin is inside the shape (SDF < 0), push outward along
        // the gradient to escape
        float originD = scene.evaluate(ox, oy, oz);
        if (originD < sdf::ACOUSTIC_ORIGIN_THRESH)
        {
            float gx, gy, gz;
            sdfGradient(scene, ox, oy, oz, gx, gy, gz);
            float pushDist = std::abs(originD) + 0.02f;
            ox += gx * pushDist;
            oy += gy * pushDist;
            oz += gz * pushDist;
        }

        float energy = 1.f;
        float totalPath = 0.f;

        for (int bounce = 0; bounce < maxBounces; ++bounce)
        {
            bool hitSurface = false;
            for (int ms = 0; ms < sdf::ACOUSTIC_MARCH_STEPS; ++ms)
            {
                float d = scene.evaluate(ox, oy, oz);
                if (d < sdf::ACOUSTIC_SURFACE_THRESH)
                {
                    hitSurface = true;
                    break;
                }
                if (totalPath > sdf::ACOUSTIC_MAX_PATH) break;
                float step = std::max(d, sdf::ACOUSTIC_MIN_STEP);
                ox += dirX * step;
                oy += dirY * step;
                oz += dirZ * step;
                totalPath += step;
            }

            if (!hitSurface || totalPath > sdf::ACOUSTIC_MAX_PATH)
                break;

            float pos = std::fmod(totalPath * distScale * 0.5f, 1.f);
            int center = static_cast<int>(pos * n) % n;
            int halfWidth = sdf::ACOUSTIC_HANN_HALF;

            for (int w = -halfWidth; w <= halfWidth; ++w)
            {
                int idx = (center + w + n) % n;
                float wt = 0.5f * (1.f + std::cos(sdf::PI * w / static_cast<float>(halfWidth)));
                table[idx] += wt * energy;
            }

            // Reflect off surface
            float nx, ny, nz;
            sdfGradient(scene, ox, oy, oz, nx, ny, nz);
            float dot = dirX * nx + dirY * ny + dirZ * nz;
            dirX -= 2.f * dot * nx;
            dirY -= 2.f * dot * ny;
            dirZ -= 2.f * dot * nz;

            // Push off surface
            ox += nx * sdf::ACOUSTIC_PUSH_OFFSET;
            oy += ny * sdf::ACOUSTIC_PUSH_OFFSET;
            oz += nz * sdf::ACOUSTIC_PUSH_OFFSET;

            // Phase 4b: surface-dependent energy decay — glancing reflections retain more
            energy *= 0.7f + 0.2f * std::abs(dot);
        }
    }

    // FIX: Warm up leaky integrator — run one full pass to establish steady-state,
    // then run again for the actual output. This eliminates the directional ramp bias.
    float acc = 0.f;
    for (int i = 0; i < n; ++i)
        acc = acc * sdf::ACOUSTIC_INTEGRATOR + table[i];
    // Now acc holds the warmed-up state — run the real pass
    for (int i = 0; i < n; ++i)
    {
        acc = acc * sdf::ACOUSTIC_INTEGRATOR + table[i];
        table[i] = acc;
    }

    removeDCAndNormalize(table);

    return table;
}

// ── Mode 3: Granular Surface Particles (curvature-based) ───────────
// FIX: Larger epsilon for curvature (0.005 vs 0.001 avoids 10^6 noise amplification),
//      DC removal added

WavetableGenerator::Wavetable WavetableGenerator::generateGranularCurvature(
    const SDFScene3D& scene,
    float /*scanRadius*/, float scanHeight, float topoMorph, float distScale,
    ScanAlgorithmData* outAlgData)
{
    Wavetable table{};
    const int n = sdf::TABLE_SIZE;

    int numGrains = sdf::GRAIN_MIN_COUNT + static_cast<int>(std::floor(topoMorph * (sdf::GRAIN_MAX_COUNT - sdf::GRAIN_MIN_COUNT)));
    numGrains = std::min(numGrains, sdf::GRAIN_MAX_COUNT);

    float windowWidth = 0.2f * (1.f - topoMorph * 0.8f);
    float invWW2 = 1.f / (windowWidth * windowWidth + 0.0001f);

    // Phase 4c: scale max frequency range with distScale
    float maxFreq = sdf::GRAIN_MAX_FREQ * (0.5f + distScale * 0.5f);

    struct Grain { float angle; float freq; };
    std::array<Grain, sdf::GRAIN_MAX_COUNT> grains{};

    constexpr float cEps = sdf::GRAIN_CURVATURE_EPS;

    // Export algorithm data
    if (outAlgData)
    {
        outAlgData->numGrains = numGrains;
        outAlgData->grainData.clear();
        outAlgData->grainData.reserve(static_cast<size_t>(numGrains));
    }

    for (int g = 0; g < numGrains; ++g)
    {
        float angle = std::fmod(g * sdf::GOLDEN_ANGLE, sdf::TWO_PI);
        float surfR = findSurfaceRadius(scene, angle, scanHeight);

        float px = std::cos(angle) * surfR;
        float pz = std::sin(angle) * surfR;
        float py = scanHeight;

        // Laplacian curvature (7 evals)
        float fc = scene.evaluate(px, py, pz);
        float curvature = (scene.evaluate(px + cEps, py, pz) + scene.evaluate(px - cEps, py, pz)
                         + scene.evaluate(px, py + cEps, pz) + scene.evaluate(px, py - cEps, pz)
                         + scene.evaluate(px, py, pz + cEps) + scene.evaluate(px, py, pz - cEps)
                         - 6.f * fc) / (cEps * cEps);

        float freq = std::clamp(2.f + std::abs(curvature) * distScale * sdf::GRAIN_CURVATURE_SCALE,
                                sdf::GRAIN_MIN_FREQ, maxFreq);
        grains[g] = { angle, freq };

        if (outAlgData)
            outAlgData->grainData.push_back({ angle, freq, curvature });
    }

    for (int i = 0; i < n; ++i)
    {
        float phase = (static_cast<float>(i) / n) * sdf::TWO_PI;
        float sum = 0.f;

        for (int g = 0; g < numGrains; ++g)
        {
            float angDist = phase - grains[g].angle;
            while (angDist > sdf::PI) angDist -= sdf::TWO_PI;
            while (angDist < -sdf::PI) angDist += sdf::TWO_PI;

            float gaussian = std::exp(-angDist * angDist * invWW2);
            sum += gaussian * std::sin(angDist * grains[g].freq);
        }

        table[i] = sum;
    }

    removeDCAndNormalize(table);

    return table;
}

// ── Mode 4: Volumetric Spectrogram ─────────────────────────────────
// FIX: Use per-height average magnitudes for true additive synthesis (constant
//      harmonic weights across cycle), DC removal added

WavetableGenerator::Wavetable WavetableGenerator::generateVolumetricSpectro(
    const SDFScene3D& scene,
    float scanRadius, float scanHeight, float topoMorph, float distScale,
    ScanAlgorithmData* outAlgData)
{
    Wavetable table{};
    const int n = sdf::TABLE_SIZE;

    int activeBins = 8 + static_cast<int>(std::floor(topoMorph * static_cast<float>(sdf::SPECTRO_HEIGHT_SLICES - 8)));
    activeBins = std::clamp(activeBins, 8, sdf::SPECTRO_HEIGHT_SLICES);

    // Shape-adaptive height range — pre-scan vertical extent over 4 angles
    float heightRange = sdf::SPECTRO_HEIGHT_RANGE;
    {
        float loSum = 0.f, hiSum = 0.f;
        constexpr int probeAngles = 4;
        for (int pa = 0; pa < probeAngles; ++pa)
        {
            float angle = static_cast<float>(pa) / probeAngles * sdf::TWO_PI;
            float dirX = std::cos(angle) * scanRadius;
            float dirZ = std::sin(angle) * scanRadius;
            float lo = scanHeight, hi = scanHeight;
            for (float dy = 0.01f; dy < 1.5f; dy += 0.02f)
            {
                if (scene.evaluate(dirX, scanHeight + dy, dirZ) < 0.3f)
                    hi = scanHeight + dy;
                if (scene.evaluate(dirX, scanHeight - dy, dirZ) < 0.3f)
                    lo = scanHeight - dy;
            }
            loSum += lo;
            hiSum += hi;
        }
        float extent = (hiSum / probeAngles - loSum / probeAngles) * 0.5f;
        if (extent > 0.05f)
            heightRange = std::clamp(extent, 0.1f, 1.0f);
    }

    // Sample SDF on cylindrical grid and compute per-height average magnitude
    std::array<float, sdf::SPECTRO_HEIGHT_SLICES> harmonicWeights{};
    harmonicWeights.fill(0.f);

    for (int a = 0; a < sdf::SPECTRO_ANGLE_SAMPLES; ++a)
    {
        float theta = (static_cast<float>(a) / sdf::SPECTRO_ANGLE_SAMPLES) * sdf::TWO_PI;
        float cx = std::cos(theta) * scanRadius;
        float cz = std::sin(theta) * scanRadius;

        for (int h = 0; h < sdf::SPECTRO_HEIGHT_SLICES; ++h)
        {
            float hy = scanHeight - heightRange
                     + (static_cast<float>(h) / (sdf::SPECTRO_HEIGHT_SLICES - 1)) * (heightRange * 2.f);
            float d = scene.evaluate(cx, hy, cz);
            harmonicWeights[h] += std::exp(-d * d * distScale * distScale * 4.f);
        }
    }

    // Normalize weights by angle count
    for (int h = 0; h < sdf::SPECTRO_HEIGHT_SLICES; ++h)
        harmonicWeights[h] /= sdf::SPECTRO_ANGLE_SAMPLES;

    // Export algorithm data
    if (outAlgData)
    {
        outAlgData->activeBins = activeBins;
        outAlgData->harmonicWeights = harmonicWeights;
    }

    // Additive synthesis with constant harmonic weights
    for (int i = 0; i < n; ++i)
    {
        float phase = (static_cast<float>(i) / n) * sdf::TWO_PI;
        float sum = 0.f;

        for (int h = 0; h < activeBins; ++h)
        {
            float harmonic = static_cast<float>(h + 1);
            sum += harmonicWeights[h] * std::sin(phase * harmonic);
        }

        table[i] = sum;
    }

    removeDCAndNormalize(table);

    return table;
}

// ── Mode 5: Field Traversal ────────────────────────────────────────
// FIX: Crossfade loop point to eliminate click from non-closing Lissajous curves

WavetableGenerator::Wavetable WavetableGenerator::generateFieldTraverse(
    const SDFScene3D& scene,
    float scanRadius, float scanHeight, float topoMorph, float distScale,
    ScanAlgorithmData* outAlgData)
{
    const int n = sdf::TABLE_SIZE;

    // Lissajous ratios morph with topoMorph (piecewise formula)
    float a, b, c, delta;
    if (topoMorph < 0.5f)
    {
        float t = topoMorph * 2.f;
        a = 1.f;
        b = 1.f + t;
        c = t * 1.5f;
        delta = t * 0.5f;
    }
    else
    {
        float t = (topoMorph - 0.5f) * 2.f;
        a = 1.f + t;
        b = 2.f + t;
        c = 1.5f + t * 3.5f;
        delta = 0.5f + t * 1.f;
    }

    // Export algorithm data
    if (outAlgData)
    {
        outAlgData->lissA = a;
        outAlgData->lissB = b;
        outAlgData->lissC = c;
        outAlgData->lissDelta = delta;
    }

    // Phase 4e: adaptive oversampling — higher ratios need more samples
    int osRate = sdf::FIELD_PATH_OVERSAMPLE + static_cast<int>(std::ceil(std::max({a, b, c}) / 2.f));
    osRate = std::min(osRate, 12); // cap to avoid huge allocations
    const int overN = n * osRate;

    // Oversample: evaluate SDF along 3D Lissajous path
    std::vector<float> oversampled(static_cast<size_t>(overN));
    for (int i = 0; i < overN; ++i)
    {
        float t = (static_cast<float>(i) / overN) * sdf::TWO_PI;
        float px = std::sin(a * t + delta) * scanRadius;
        float pz = std::sin(b * t) * scanRadius;
        float py = scanHeight + std::sin(c * t) * scanRadius * 0.4f;

        float d = scene.evaluate(px, py, pz);
        oversampled[static_cast<size_t>(i)] = std::tanh(d * distScale);
    }

    // Decimate with triangular (Bartlett) window for better anti-aliasing than box
    Wavetable table{};
    for (int i = 0; i < n; ++i)
    {
        float sum = 0.f;
        float wSum = 0.f;
        int base = i * osRate;
        for (int j = 0; j < osRate; ++j)
        {
            // Triangular weight: peak at center of the window
            float w = 1.f - std::abs(2.f * j - (osRate - 1)) / static_cast<float>(osRate);
            sum += oversampled[static_cast<size_t>(base + j)] * w;
            wSum += w;
        }
        table[i] = sum / wSum;
    }

    crossfadeLoopPoint(table, sdf::FIELD_CROSSFADE_LEN);

    removeDCAndNormalize(table);

    return table;
}

// ── Radix-2 FFT (in-place, Cooley-Tukey) ──────────────────────────

void WavetableGenerator::fft(std::vector<std::complex<float>>& data, bool inverse)
{
    int n = static_cast<int>(data.size());
    jassert(n > 0 && (n & (n - 1)) == 0); // must be power of 2

    // Bit-reversal permutation
    for (int i = 1, j = 0; i < n; ++i)
    {
        int bit = n >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j ^= bit;
        if (i < j) std::swap(data[i], data[j]);
    }

    // Butterfly passes
    for (int len = 2; len <= n; len <<= 1)
    {
        float angle = (inverse ? 1.f : -1.f) * sdf::TWO_PI / static_cast<float>(len);
        std::complex<float> wn(std::cos(angle), std::sin(angle));

        for (int i = 0; i < n; i += len)
        {
            std::complex<float> w(1.f, 0.f);
            for (int j = 0; j < len / 2; ++j)
            {
                auto u = data[i + j];
                auto v = data[i + j + len / 2] * w;
                data[i + j] = u + v;
                data[i + j + len / 2] = u - v;
                w *= wn;
            }
        }
    }

    if (inverse)
    {
        float invN = 1.f / static_cast<float>(n);
        for (auto& x : data) x *= invN;
    }
}

// ── Mip-Mapped Wavetable Generation ────────────────────────────────

MipMappedWavetable WavetableGenerator::generateMipMap(const Wavetable& base)
{
    MipMappedWavetable mip;
    constexpr int N = sdf::TABLE_SIZE;

    // FFT the base table
    std::vector<std::complex<float>> spectrum(N);
    for (int i = 0; i < N; ++i)
        spectrum[i] = std::complex<float>(base[i], 0.f);
    fft(spectrum, false);

    // Level 0 = full bandwidth (copy of base)
    mip.tables[0] = base;

    // For each octave level: zero bins above TABLE_SIZE / 2^(n+1), IFFT back
    for (int level = 1; level < sdf::MIP_LEVELS; ++level)
    {
        int maxBin = N / (1 << (level + 1));
        if (maxBin < 1) maxBin = 1;

        std::vector<std::complex<float>> filtered(N);
        filtered[0] = spectrum[0]; // DC

        for (int k = 1; k < N; ++k)
        {
            // Keep bins 1..maxBin and their mirror N-maxBin..N-1
            if (k <= maxBin || k >= N - maxBin)
                filtered[k] = spectrum[k];
            else
                filtered[k] = std::complex<float>(0.f, 0.f);
        }

        fft(filtered, true);

        for (int i = 0; i < N; ++i)
            mip.tables[level][i] = filtered[i].real();
    }

    return mip;
}
