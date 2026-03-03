#include <juce_core/juce_core.h>
#include "DSP/MeshImporter.h"
#include <cmath>

class TestMeshImporter : public juce::UnitTest
{
public:
    TestMeshImporter() : UnitTest("MeshImporter") {}

    void runTest() override
    {
        beginTest("ptTriDistSq - point at vertex");
        {
            // Point exactly at vertex A of triangle at origin
            float d2 = MeshImporter::ptTriDistSq(
                0.f, 0.f, 0.f,   // point
                0.f, 0.f, 0.f,   // A
                1.f, 0.f, 0.f,   // B
                0.f, 1.f, 0.f);  // C
            expectWithinAbsoluteError(d2, 0.f, 1e-6f);
        }

        beginTest("ptTriDistSq - point on edge");
        {
            // Point at midpoint of edge AB
            float d2 = MeshImporter::ptTriDistSq(
                0.5f, 0.f, 0.f,
                0.f, 0.f, 0.f,
                1.f, 0.f, 0.f,
                0.f, 1.f, 0.f);
            expectWithinAbsoluteError(d2, 0.f, 1e-6f);
        }

        beginTest("ptTriDistSq - point above face");
        {
            // Point directly above triangle centroid
            float d2 = MeshImporter::ptTriDistSq(
                0.25f, 0.25f, 1.f,  // point (above centroid-ish)
                0.f, 0.f, 0.f,
                1.f, 0.f, 0.f,
                0.f, 1.f, 0.f);
            // Closest point should be on the triangle face, distance ~1.0
            expectWithinAbsoluteError(d2, 1.f, 1e-5f);
        }

        beginTest("ptTriDistSq - point outside edge");
        {
            // Point at (2,0,0) - closest is vertex B at (1,0,0)
            float d2 = MeshImporter::ptTriDistSq(
                2.f, 0.f, 0.f,
                0.f, 0.f, 0.f,
                1.f, 0.f, 0.f,
                0.f, 1.f, 0.f);
            expectWithinAbsoluteError(d2, 1.f, 1e-5f);
        }

        beginTest("normalizeMesh bounds");
        {
            TriMesh mesh;
            mesh.vertices = { {0,0,0}, {10,0,0}, {0,10,0}, {0,0,10} };
            mesh.faces = { {0,1,2}, {0,1,3} };
            MeshImporter::normalizeMesh(mesh);

            // After normalization, all vertices should be within [-1,1]
            for (const auto& v : mesh.vertices)
            {
                expect(v.x >= -1.f && v.x <= 1.f);
                expect(v.y >= -1.f && v.y <= 1.f);
                expect(v.z >= -1.f && v.z <= 1.f);
            }

            // Extent should be about 1.4 / original_extent = 0.14 * original
            float maxExt = 0.f;
            for (const auto& v : mesh.vertices)
            {
                maxExt = std::max(maxExt, std::abs(v.x));
                maxExt = std::max(maxExt, std::abs(v.y));
                maxExt = std::max(maxExt, std::abs(v.z));
            }
            expect(maxExt <= 0.71f); // 1.4/2 = 0.7, max vertex at edge
        }

        beginTest("meshToVoxelSDF resolution");
        {
            // Simple 2-triangle quad
            TriMesh mesh;
            mesh.vertices = {
                {-0.5f, -0.5f, 0.f},
                { 0.5f, -0.5f, 0.f},
                { 0.5f,  0.5f, 0.f},
                {-0.5f,  0.5f, 0.f}
            };
            mesh.faces = { {0,1,2}, {0,2,3} };

            auto sdf = MeshImporter::meshToVoxelSDF(mesh, 8);
            expect(sdf != nullptr);
            expectEquals(sdf->res, 8);
            expectEquals(static_cast<int>(sdf->grid.size()), 8 * 8 * 8);
        }

        beginTest("meshToVoxelSDF sign correctness");
        {
            // Unit cube (6 faces, 8 vertices) - inside should be negative
            TriMesh mesh;
            float h = 0.5f;
            mesh.vertices = {
                {-h,-h,-h}, { h,-h,-h}, { h, h,-h}, {-h, h,-h}, // front face
                {-h,-h, h}, { h,-h, h}, { h, h, h}, {-h, h, h}  // back face
            };
            // 12 triangles for 6 faces (outward normals)
            mesh.faces = {
                {0,2,1}, {0,3,2}, // front (-z)
                {4,5,6}, {4,6,7}, // back (+z)
                {0,1,5}, {0,5,4}, // bottom (-y)
                {2,3,7}, {2,7,6}, // top (+y)
                {0,4,7}, {0,7,3}, // left (-x)
                {1,2,6}, {1,6,5}  // right (+x)
            };

            auto sdf = MeshImporter::meshToVoxelSDF(mesh, 8);
            expect(sdf != nullptr);

            // Center voxel should be negative (inside)
            float center = sdf->sample(0.f, 0.f, 0.f);
            expect(center < 0.f);

            // Corner should be positive (outside)
            float corner = sdf->sample(0.9f, 0.9f, 0.9f);
            expect(corner > 0.f);
        }

        beginTest("No NaN or Inf in voxelized grid");
        {
            TriMesh mesh;
            mesh.vertices = {
                {0.f, 0.f, 0.f},
                {1.f, 0.f, 0.f},
                {0.f, 1.f, 0.f}
            };
            mesh.faces = { {0, 1, 2} };

            auto sdf = MeshImporter::meshToVoxelSDF(mesh, 8);
            expect(sdf != nullptr);

            bool hasNanOrInf = false;
            for (float val : sdf->grid)
            {
                if (std::isnan(val) || std::isinf(val))
                {
                    hasNanOrInf = true;
                    break;
                }
            }
            expect(!hasNanOrInf);
        }
    }
};

static TestMeshImporter testMeshImporter;
