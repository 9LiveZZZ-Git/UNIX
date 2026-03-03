#include "SDFScene3D.h"

float SDFScene3D::evalShape(ShapeType shape, float x, float y, float z, float size,
                             float sfM_, float sfN1_, float sfN2_, float sfN3_)
{
    switch (shape)
    {
        case ShapeType::Sphere:      return sdf::sphere(x, y, z, size);
        case ShapeType::Box:         return sdf::box(x, y, z, size * 0.75f);
        case ShapeType::Torus:       return sdf::torus(x, y, z, size * 0.65f, size * 0.25f);
        case ShapeType::Cylinder:    return sdf::cylinder(x, y, z, size * 0.5f, size * 0.8f);
        case ShapeType::Octahedron:  return sdf::octahedron(x, y, z, size);
        case ShapeType::Capsule:     return sdf::capsule(x, y, z, size, size * 0.3f);
        case ShapeType::RoundBox:    return sdf::roundBox(x, y, z, size, size, size, size * 0.15f);
        case ShapeType::HexPrism:    return sdf::hexPrism(x, y, z, size, size * 0.5f);
        case ShapeType::Torus82:     return sdf::torus82(x, y, z, size, size * 0.25f);
        case ShapeType::Torus88:     return sdf::torus88(x, y, z, size, size * 0.25f);
        case ShapeType::SuperFormula: return sdf::superFormula(x, y, z, sfM_, sfN1_, sfN2_, sfN3_, size);
        default:                     return sdf::sphere(x, y, z, size);
    }
}

float SDFScene3D::evalShapeOrCustom(ShapeType shape, float x, float y, float z, float size) const
{
    if (shape == ShapeType::Custom && voxelSDF)
        return voxelSDF->sample(x, y, z);

    return evalShape(shape, x, y, z, size, sfM, sfN1, sfN2, sfN3);
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

    float d1 = evalShapeOrCustom(shape1, sx, sy, sz, size1);
    float d2 = evalShapeOrCustom(shape2, sx - offsetX, sy - offsetY, sz, size2);

    float d;
    switch (operation)
    {
        case OperationType::SmoothUnion:        d = sdf::ops::smoothMin(d1, d2, smoothK); break;
        case OperationType::Union:              d = std::min(d1, d2); break;
        case OperationType::Intersection:       d = std::max(d1, d2); break;
        case OperationType::Subtraction:        d = std::max(d1, -d2); break;
        case OperationType::SmoothIntersection: d = sdf::ops::smoothIntersection(d1, d2, smoothK); break;
        case OperationType::SmoothSubtraction:  d = sdf::ops::smoothSubtraction(d1, d2, smoothK); break;
        case OperationType::ChamferUnion:       d = sdf::ops::chamferUnion(d1, d2, smoothK); break;
        case OperationType::ChamferIntersection: d = sdf::ops::chamferIntersection(d1, d2, smoothK); break;
        case OperationType::ChamferSubtraction: d = sdf::ops::chamferSubtraction(d1, d2, smoothK); break;
        case OperationType::StairsUnion:        d = sdf::ops::stairsUnion(d1, d2, smoothK, stairCount); break;
        case OperationType::Pipe:               d = sdf::ops::pipe(d1, d2, smoothK); break;
        default:                                d = std::min(d1, d2); break;
    }

    // Onion shell
    if (onionEnable)
        d = std::abs(d) - onionThickness;

    return d;
}
