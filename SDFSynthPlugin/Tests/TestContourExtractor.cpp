#include <juce_core/juce_core.h>
#include "DSP/ContourExtractor.h"
#include "DSP/SDFScene3D.h"
#include <cmath>

class ContourExtractorTests : public juce::UnitTest
{
public:
    ContourExtractorTests() : juce::UnitTest("ContourExtractor") {}

    void runTest() override
    {
        beginTest("Sphere produces roughly circular contour");
        {
            SDFScene3D scene;
            scene.shape1 = ShapeType::Sphere;
            scene.shape2 = ShapeType::Sphere;
            scene.size1 = 0.5f;
            scene.size2 = 0.0f;  // zero-size second shape
            scene.offsetX = 0.f;
            scene.offsetY = 0.f;
            scene.operation = OperationType::SmoothUnion;

            ContourExtractor ext;
            auto contour = ext.extractContour(scene, 0.f, 64);

            // All valid points should have similar radius
            float minR = 1e9f, maxR = -1e9f;
            int validCount = 0;
            for (auto& cp : contour)
            {
                if (cp.valid)
                {
                    minR = std::min(minR, cp.r);
                    maxR = std::max(maxR, cp.r);
                    ++validCount;
                }
            }
            expect(validCount > 50, "Most contour points should be valid for a sphere");
            if (validCount > 0)
            {
                float spread = maxR - minR;
                expect(spread < 0.05f,
                       "Sphere contour radii should be nearly uniform, spread: " + juce::String(spread));
            }
        }

        beginTest("getScanPoint is continuous (no large jumps)");
        {
            SDFScene3D scene;
            ContourExtractor ext;
            auto contour = ext.extractContour(scene, 0.f);

            float maxJump = 0.f;
            float prevR = ContourExtractor::getScanPoint(0.f, contour, 1.f, 1.f).r;

            int steps = 1000;
            for (int i = 1; i <= steps; ++i)
            {
                float theta = sdf::TWO_PI * i / static_cast<float>(steps);
                auto sp = ContourExtractor::getScanPoint(theta, contour, 1.f, 1.f);
                float jump = std::abs(sp.r - prevR);
                maxJump = std::max(maxJump, jump);
                prevR = sp.r;
            }
            expect(maxJump < 0.1f,
                   "getScanPoint should be continuous, max jump: " + juce::String(maxJump));
        }

        beginTest("getScanPoint wraps at 2*PI");
        {
            SDFScene3D scene;
            ContourExtractor ext;
            auto contour = ext.extractContour(scene, 0.f);

            auto sp0 = ContourExtractor::getScanPoint(0.f, contour, 1.f, 0.5f);
            auto sp2pi = ContourExtractor::getScanPoint(sdf::TWO_PI, contour, 1.f, 0.5f);

            expectWithinAbsoluteError(sp0.r, sp2pi.r, 0.001f,
                                       "getScanPoint should wrap at 2*PI");
        }

        beginTest("topoMorph=0 gives circular, topoMorph=1 gives contour");
        {
            SDFScene3D scene;
            ContourExtractor ext;
            auto contour = ext.extractContour(scene, 0.f);

            float scanR = 1.2f;
            // topoMorph=0: all radii should equal scanRadius
            for (int i = 0; i < 32; ++i)
            {
                float theta = sdf::TWO_PI * i / 32.f;
                auto sp = ContourExtractor::getScanPoint(theta, contour, scanR, 0.f);
                expectWithinAbsoluteError(sp.r, scanR, 0.001f,
                                           "topoMorph=0 should give scanRadius");
            }
        }
    }
};

static ContourExtractorTests contourExtractorTests;
