#include "MeshImporter.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <limits>

std::shared_ptr<VoxelSDF> MeshImporter::importOBJ(const std::string& filePath, int resolution)
{
    TriMesh mesh;
    if (!parseOBJ(filePath, mesh))
        return nullptr;
    if (mesh.faces.empty())
        return nullptr;

    normalizeMesh(mesh);
    return meshToVoxelSDF(mesh, resolution);
}

bool MeshImporter::parseOBJ(const std::string& filePath, TriMesh& outMesh)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return false;

    outMesh.vertices.clear();
    outMesh.faces.clear();

    std::string line;
    while (std::getline(file, line))
    {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v")
        {
            TriMesh::Vec3 v{};
            iss >> v.x >> v.y >> v.z;
            outMesh.vertices.push_back(v);
        }
        else if (prefix == "f")
        {
            // Parse face indices - handles formats: "f 1 2 3", "f 1/2 3/4 5/6", "f 1/2/3 4/5/6 7/8/9"
            std::vector<int> indices;
            std::string token;
            while (iss >> token)
            {
                // Extract vertex index (before first '/')
                std::string idxStr;
                for (char c : token)
                {
                    if (c == '/')
                        break;
                    idxStr += c;
                }

                if (idxStr.empty())
                    continue;

                int idx = std::stoi(idxStr);
                int numVerts = static_cast<int>(outMesh.vertices.size());

                // Support negative indices (relative to current vertex count)
                if (idx < 0)
                    idx = numVerts + idx + 1;

                // Convert 1-based to 0-based
                idx -= 1;

                if (idx >= 0 && idx < numVerts)
                    indices.push_back(idx);
            }

            // Fan triangulation for polygons with 3+ vertices
            for (size_t i = 2; i < indices.size(); ++i)
            {
                outMesh.faces.push_back({ indices[0], indices[i - 1], indices[i] });
            }
        }
    }

    return !outMesh.vertices.empty();
}

void MeshImporter::normalizeMesh(TriMesh& mesh)
{
    if (mesh.vertices.empty())
        return;

    // Find bounding box
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& v : mesh.vertices)
    {
        minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
        minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
        minZ = std::min(minZ, v.z); maxZ = std::max(maxZ, v.z);
    }

    // Center at origin
    float cx = (minX + maxX) * 0.5f;
    float cy = (minY + maxY) * 0.5f;
    float cz = (minZ + maxZ) * 0.5f;

    // Scale to fit within [-1,1]^3 with margin
    float extX = maxX - minX;
    float extY = maxY - minY;
    float extZ = maxZ - minZ;
    float extent = std::max({ extX, extY, extZ });

    float scale = (extent > 1e-8f) ? (1.4f / extent) : 1.0f;

    for (auto& v : mesh.vertices)
    {
        v.x = (v.x - cx) * scale;
        v.y = (v.y - cy) * scale;
        v.z = (v.z - cz) * scale;
    }
}

// 7-region Voronoi point-to-triangle distance squared
float MeshImporter::ptTriDistSq(float px, float py, float pz,
                                  float ax, float ay, float az,
                                  float bx, float by, float bz,
                                  float cx, float cy, float cz)
{
    // Edge vectors
    float e0x = bx - ax, e0y = by - ay, e0z = bz - az;
    float e1x = cx - ax, e1y = cy - ay, e1z = cz - az;
    float vx = px - ax, vy = py - ay, vz = pz - az;

    float d00 = e0x * e0x + e0y * e0y + e0z * e0z;
    float d01 = e0x * e1x + e0y * e1y + e0z * e1z;
    float d11 = e1x * e1x + e1y * e1y + e1z * e1z;
    float d20 = vx * e0x + vy * e0y + vz * e0z;
    float d21 = vx * e1x + vy * e1y + vz * e1z;

    float denom = d00 * d11 - d01 * d01;
    float s, t;

    if (std::abs(denom) < 1e-12f)
    {
        s = 0.f;
        t = 0.f;
    }
    else
    {
        s = (d11 * d20 - d01 * d21) / denom;
        t = (d00 * d21 - d01 * d20) / denom;
    }

    // Clamp to triangle region
    if (s < 0.f)
    {
        s = 0.f;
        t = std::clamp(d21 / std::max(d11, 1e-12f), 0.f, 1.f);
    }
    else if (t < 0.f)
    {
        t = 0.f;
        s = std::clamp(d20 / std::max(d00, 1e-12f), 0.f, 1.f);
    }
    else if (s + t > 1.f)
    {
        // Project onto edge BC
        float ex = cx - bx, ey = cy - by, ez = cz - bz;
        float wx = px - bx, wy = py - by, wz = pz - bz;
        float edgeLen = ex * ex + ey * ey + ez * ez;
        float proj = (wx * ex + wy * ey + wz * ez) / std::max(edgeLen, 1e-12f);
        proj = std::clamp(proj, 0.f, 1.f);
        s = 1.f - proj;
        t = proj;
    }

    // Closest point
    float qx = ax + s * e0x + t * e1x;
    float qy = ay + s * e0y + t * e1y;
    float qz = az + s * e0z + t * e1z;

    float dx = px - qx, dy = py - qy, dz = pz - qz;
    return dx * dx + dy * dy + dz * dz;
}

std::shared_ptr<VoxelSDF> MeshImporter::meshToVoxelSDF(const TriMesh& mesh, int res)
{
    auto sdf = std::make_shared<VoxelSDF>();
    sdf->res = res;
    sdf->grid.resize(static_cast<size_t>(res * res * res), 1.5f);

    // Precompute face normals and centroids
    struct FaceInfo
    {
        float nx, ny, nz; // normal
        float cx, cy, cz; // centroid
    };
    std::vector<FaceInfo> faceInfos(mesh.faces.size());

    for (size_t fi = 0; fi < mesh.faces.size(); ++fi)
    {
        const auto& f = mesh.faces[fi];
        const auto& a = mesh.vertices[static_cast<size_t>(f.a)];
        const auto& b = mesh.vertices[static_cast<size_t>(f.b)];
        const auto& c = mesh.vertices[static_cast<size_t>(f.c)];

        // Edge vectors
        float e0x = b.x - a.x, e0y = b.y - a.y, e0z = b.z - a.z;
        float e1x = c.x - a.x, e1y = c.y - a.y, e1z = c.z - a.z;

        // Cross product for normal
        float nx = e0y * e1z - e0z * e1y;
        float ny = e0z * e1x - e0x * e1z;
        float nz = e0x * e1y - e0y * e1x;
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (len > 1e-10f) { nx /= len; ny /= len; nz /= len; }

        faceInfos[fi] = { nx, ny, nz,
                           (a.x + b.x + c.x) / 3.f,
                           (a.y + b.y + c.y) / 3.f,
                           (a.z + b.z + c.z) / 3.f };
    }

    // For each voxel, find nearest triangle distance and determine sign
    for (int iz = 0; iz < res; ++iz)
    {
        float z = -1.f + 2.f * static_cast<float>(iz) / static_cast<float>(res - 1);
        for (int iy = 0; iy < res; ++iy)
        {
            float y = -1.f + 2.f * static_cast<float>(iy) / static_cast<float>(res - 1);
            for (int ix = 0; ix < res; ++ix)
            {
                float x = -1.f + 2.f * static_cast<float>(ix) / static_cast<float>(res - 1);

                float minDistSq = std::numeric_limits<float>::max();
                int nearestFace = -1;

                for (size_t fi = 0; fi < mesh.faces.size(); ++fi)
                {
                    const auto& f = mesh.faces[fi];
                    const auto& va = mesh.vertices[static_cast<size_t>(f.a)];
                    const auto& vb = mesh.vertices[static_cast<size_t>(f.b)];
                    const auto& vc = mesh.vertices[static_cast<size_t>(f.c)];

                    float d2 = ptTriDistSq(x, y, z,
                                           va.x, va.y, va.z,
                                           vb.x, vb.y, vb.z,
                                           vc.x, vc.y, vc.z);
                    if (d2 < minDistSq)
                    {
                        minDistSq = d2;
                        nearestFace = static_cast<int>(fi);
                    }
                }

                float dist = std::sqrt(minDistSq);

                // Determine sign: dot(p - centroid, faceNormal)
                if (nearestFace >= 0)
                {
                    const auto& info = faceInfos[static_cast<size_t>(nearestFace)];
                    float dx = x - info.cx;
                    float dy = y - info.cy;
                    float dz = z - info.cz;
                    float dot = dx * info.nx + dy * info.ny + dz * info.nz;
                    if (dot < 0.f)
                        dist = -dist;
                }

                sdf->grid[static_cast<size_t>(ix + iy * res + iz * res * res)] = dist;
            }
        }
    }

    return sdf;
}
