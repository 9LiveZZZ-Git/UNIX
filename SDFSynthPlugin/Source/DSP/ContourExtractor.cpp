#include "ContourExtractor.h"
#include <cmath>

std::vector<ContourPoint> ContourExtractor::extractContour(const SDFScene3D& scene, float scanHeight,
                                                            int numSamples) const
{
    std::vector<ContourPoint> contour(numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        float theta = (static_cast<float>(i) / numSamples) * sdf::TWO_PI;
        float dirX = std::cos(theta);
        float dirZ = std::sin(theta);

        float prevD = scene.evaluate(0.f, scanHeight, 0.f);
        float lastCrossing = -1.f;

        for (int s = 1; s <= sdf::CONTOUR_STEPS; ++s)
        {
            float r = (static_cast<float>(s) / sdf::CONTOUR_STEPS) * sdf::MAX_RADIUS;
            float d = scene.evaluate(dirX * r, scanHeight, dirZ * r);

            bool signChange = (prevD < 0.f) != (d < 0.f);
            if (signChange)
            {
                float rLo = (static_cast<float>(s - 1) / sdf::CONTOUR_STEPS) * sdf::MAX_RADIUS;
                float rHi = r;
                float dLo = prevD;
                float dHi = d;

                // Illinois method (regula falsi with anti-stall modification)
                for (int b = 0; b < sdf::BINARY_ITERS; ++b)
                {
                    float rMid = rLo - dLo * (rHi - rLo) / (dHi - dLo);
                    float dMid = scene.evaluate(dirX * rMid, scanHeight, dirZ * rMid);

                    if ((dMid < 0.f) == (dLo < 0.f))
                    {
                        rLo = rMid;
                        dLo = dMid;
                        dHi *= 0.5f;  // Illinois: halve retained side
                    }
                    else
                    {
                        rHi = rMid;
                        dHi = dMid;
                        dLo *= 0.5f;
                    }
                }
                lastCrossing = rLo - dLo * (rHi - rLo) / (dHi - dLo);
            }
            prevD = d;
        }

        if (lastCrossing >= 0.f)
        {
            contour[i] = { theta, lastCrossing, true,
                           std::cos(theta) * lastCrossing,
                           std::sin(theta) * lastCrossing };
        }
        else
        {
            contour[i] = { theta, 0.f, false, 0.f, 0.f };
        }
    }

    return contour;
}

ContourExtractor::ScanPoint ContourExtractor::getScanPoint(float theta,
    const std::vector<ContourPoint>& contour, float scanRadius, float topoMorph)
{
    int n = static_cast<int>(contour.size());
    float ci = (theta / sdf::TWO_PI) * n;
    int i0 = static_cast<int>(std::floor(ci)) % n;
    if (i0 < 0) i0 += n;
    float frac = ci - std::floor(ci);

    // 4-point cubic Hermite interpolation of contour radii
    int im1 = (i0 - 1 + n) % n;
    int i1 = (i0 + 1) % n;
    int i2 = (i0 + 2) % n;
    float a = contour[im1].r, b = contour[i0].r;
    float c = contour[i1].r, d = contour[i2].r;
    float cb = c - b, ab = a - b, db = d - b;
    float cr = b + 0.5f * frac * (cb + frac * (cb - ab + frac * (3.f * (ab - cb) + db)));

    float r = scanRadius * (1.f - topoMorph) + cr * topoMorph;

    return { std::cos(theta) * r, std::sin(theta) * r, r };
}
