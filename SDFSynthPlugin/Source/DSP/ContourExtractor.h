#pragma once
#include "SDFScene3D.h"
#include "Constants.h"
#include <vector>
#include <cmath>

struct ContourPoint
{
    float theta = 0.f;
    float r = 0.f;
    bool valid = false;
    float x = 0.f;
    float z = 0.f;
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
