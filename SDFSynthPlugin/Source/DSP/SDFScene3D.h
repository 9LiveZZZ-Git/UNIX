#pragma once
#include "SDFPrimitives3D.h"
#include "SDFMath.h"
#include "VoxelSDF.h"
#include <algorithm>
#include <cmath>
#include <memory>

enum class ShapeType { Sphere = 0, Box, Torus, Cylinder, Octahedron, Custom,
                       Capsule, RoundBox, HexPrism, Torus82, Torus88, SuperFormula };
enum class OperationType { SmoothUnion = 0, Union, Intersection, Subtraction,
                           SmoothIntersection, SmoothSubtraction,
                           ChamferUnion, ChamferIntersection, ChamferSubtraction,
                           StairsUnion, Pipe };

class SDFScene3D
{
public:
    ShapeType shape1 = ShapeType::Sphere;
    ShapeType shape2 = ShapeType::Torus;
    OperationType operation = OperationType::SmoothUnion;

    float size1 = 0.4f;
    float size2 = 0.35f;
    float offsetX = 0.45f;
    float offsetY = 0.0f;
    float smoothK = 0.5f;
    float twist = 0.0f;

    // SuperFormula params
    float sfM = 6.f, sfN1 = 1.f, sfN2 = 1.f, sfN3 = 1.f;

    // Onion shell
    bool onionEnable = false;
    float onionThickness = 0.05f;

    // Stairs operation step count
    float stairCount = 4.f;

    std::shared_ptr<const VoxelSDF> voxelSDF;

    float evaluate(float x, float y, float z) const;

private:
    static float evalShape(ShapeType shape, float x, float y, float z, float size,
                           float sfM = 6.f, float sfN1 = 1.f, float sfN2 = 1.f, float sfN3 = 1.f);
    float evalShapeOrCustom(ShapeType shape, float x, float y, float z, float size) const;
};
