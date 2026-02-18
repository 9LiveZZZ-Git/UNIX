#include "SDFScene3D.h"

float SDFScene3D::evalShape(ShapeType shape, float x, float y, float z, float size)
{
    switch (shape)
    {
        case ShapeType::Sphere:     return sdf::sphere(x, y, z, size);
        case ShapeType::Box:        return sdf::box(x, y, z, size * 0.75f);
        case ShapeType::Torus:      return sdf::torus(x, y, z, size * 0.65f, size * 0.25f);
        case ShapeType::Cylinder:   return sdf::cylinder(x, y, z, size * 0.5f, size * 0.8f);
        case ShapeType::Octahedron: return sdf::octahedron(x, y, z, size);
        default:                    return sdf::sphere(x, y, z, size);
    }
}

float SDFScene3D::evaluate(float x, float y, float z) const
{
    float sx = x, sy = y, sz = z;

    if (twist > 0.01f)
    {
        float c = std::cos(twist * sy);
        float s = std::sin(twist * sy);
        float nx = sx * c - sz * s;
        float nz = sx * s + sz * c;
        sx = nx;
        sz = nz;
    }

    float d1 = evalShape(shape1, sx, sy, sz, size1);
    float d2 = evalShape(shape2, sx - offsetX, sy - offsetY, sz, size2);

    switch (operation)
    {
        case OperationType::SmoothUnion:  return sdf::ops::smoothMin(d1, d2, smoothK);
        case OperationType::Union:        return std::min(d1, d2);
        case OperationType::Intersection: return std::max(d1, d2);
        case OperationType::Subtraction:  return std::max(d1, -d2);
    }
    return std::min(d1, d2);
}
