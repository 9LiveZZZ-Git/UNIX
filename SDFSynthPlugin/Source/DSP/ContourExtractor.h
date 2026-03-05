#pragma once
#include "SDFScene3D.h"
#include "Constants.h"
#include <vector>
#include <cmath>

struct ContourPoint
{
    float theta = 0.f;
    float r = 0.f;        // outermost surface crossing radius
    bool valid = false;
    float x = 0.f;        // cos(theta) * r
    float z = 0.f;        // sin(theta) * r
    float innerR = -1.f;  // first crossing radius when origin is outside (hole detection), -1 if no hole
};

class ContourExtractor
{
public:
    std::vector<ContourPoint> extractContour(const SDFScene3D& scene, float scanHeight,
                                              int numSamples = sdf::CONTOUR_RES) const;

    struct ScanPoint { float x; float z; float r; };

    static ScanPoint getScanPoint(float theta, const std::vector<ContourPoint>& contour,
                                   float scanRadius, float topoMorph);
};
