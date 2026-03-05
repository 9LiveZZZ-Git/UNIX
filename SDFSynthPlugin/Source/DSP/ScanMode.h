#pragma once

enum class ScanMode : int {
    Contour = 0, RayMarchSonify = 1, AcousticTrace = 2,
    GranularCurvature = 3, VolumetricSpectro = 4
};
constexpr int NUM_SCAN_MODES = 5;
