#pragma once
#include "VoxelSDF.h"
#include <string>
#include <memory>

struct TriMesh
{
    struct Vec3 { float x, y, z; };
    struct Tri  { int a, b, c; };

    std::vector<Vec3> vertices;
    std::vector<Tri>  faces;
};

class MeshImporter
{
public:
    static std::shared_ptr<VoxelSDF> importOBJ(const std::string& filePath, int resolution = 24);

    static bool parseOBJ(const std::string& filePath, TriMesh& outMesh);
    static void normalizeMesh(TriMesh& mesh);
    static float ptTriDistSq(float px, float py, float pz,
                              float ax, float ay, float az,
                              float bx, float by, float bz,
                              float cx, float cy, float cz);
    static std::shared_ptr<VoxelSDF> meshToVoxelSDF(const TriMesh& mesh, int res);
};
