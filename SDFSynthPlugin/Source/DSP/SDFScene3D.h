#pragma once
#include "SDFPrimitives3D.h"
#include "SDFMath.h"
#include <algorithm>
#include <cmath>

enum class ShapeType { Sphere = 0, Box, Torus, Cylinder, Octahedron, Custom };
enum class OperationType { SmoothUnion = 0, Union, Intersection, Subtraction };

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

    float evaluate(float x, float y, float z) const;

private:
    static float evalShape(ShapeType shape, float x, float y, float z, float size);
};
