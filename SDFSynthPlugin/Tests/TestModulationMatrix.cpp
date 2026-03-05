#include <juce_core/juce_core.h>
#include "DSP/ModulationMatrix.h"
#include <cmath>

class ModulationMatrixTest : public juce::UnitTest
{
public:
    ModulationMatrixTest() : juce::UnitTest("ModulationMatrix") {}

    void runTest() override
    {
        beginTest("Lock-free publish/acquire cycle");
        {
            ModulationMatrix mm;

            // Set a slot on GUI thread side
            ModSlot slot;
            slot.source = ModSource::LFO1;
            slot.destIndex = 5;
            slot.depth = 0.7f;
            slot.active = true;
            mm.setSlot(0, slot);
            mm.publish();

            // Audio thread acquires
            mm.acquire();
            auto& slots = mm.getSlots();
            expect(slots[0].active, "Slot 0 should be active after publish/acquire");
            expect(slots[0].source == ModSource::LFO1, "Source should be LFO1");
            expect(slots[0].destIndex == 5, "Dest index should be 5");
            expect(std::abs(slots[0].depth - 0.7f) < 0.001f, "Depth should be 0.7");
        }

        beginTest("computeModulatedNorm clamps to [0, 1]");
        {
            ModulationMatrix mm;

            ModSlot slot;
            slot.source = ModSource::Envelope;
            slot.destIndex = 0;
            slot.depth = 1.0f;
            slot.active = true;
            mm.setSlot(0, slot);
            mm.publish();
            mm.acquire();

            mm.setSourceValue(ModSource::Envelope, 1.0f);

            // Base 0.8 + 1.0 * 1.0 = 1.8 → clamped to 1.0
            float result = mm.computeModulatedNorm(0, 0.8f);
            expect(std::abs(result - 1.0f) < 0.001f, "Should clamp to 1.0 (got " + juce::String(result) + ")");

            // Negative depth: base 0.2 + 1.0 * (-1.0) = -0.8 → clamped to 0.0
            ModSlot negSlot;
            negSlot.source = ModSource::Envelope;
            negSlot.destIndex = 1;
            negSlot.depth = -1.0f;
            negSlot.active = true;
            mm.setSlot(1, negSlot);
            mm.publish();
            mm.acquire();

            float result2 = mm.computeModulatedNorm(1, 0.2f);
            expect(std::abs(result2) < 0.001f, "Should clamp to 0.0 (got " + juce::String(result2) + ")");
        }

        beginTest("Multiple sources sum correctly on same destination");
        {
            ModulationMatrix mm;

            // Route 1: Envelope → dest 3, depth 0.2
            ModSlot s1;
            s1.source = ModSource::Envelope;
            s1.destIndex = 3;
            s1.depth = 0.2f;
            s1.active = true;
            mm.setSlot(0, s1);

            // Route 2: LFO1 → dest 3, depth 0.1
            ModSlot s2;
            s2.source = ModSource::LFO1;
            s2.destIndex = 3;
            s2.depth = 0.1f;
            s2.active = true;
            mm.setSlot(1, s2);

            mm.publish();
            mm.acquire();

            mm.setSourceValue(ModSource::Envelope, 1.0f);
            mm.setSourceValue(ModSource::LFO1, 0.5f);

            // base 0.5 + (1.0*0.2) + (0.5*0.1) = 0.5 + 0.2 + 0.05 = 0.75
            float result = mm.computeModulatedNorm(3, 0.5f);
            expect(std::abs(result - 0.75f) < 0.01f, "Multi-source sum should be 0.75 (got "
                   + juce::String(result) + ")");
        }

        beginTest("Per-voice offset computation");
        {
            ModulationMatrix mm;

            // Velocity → dest 10, depth 0.5
            ModSlot velSlot;
            velSlot.source = ModSource::Velocity;
            velSlot.destIndex = 10;
            velSlot.depth = 0.5f;
            velSlot.active = true;
            mm.setSlot(0, velSlot);

            // KeyTrack → dest 10, depth 0.3
            ModSlot keySlot;
            keySlot.source = ModSource::KeyTrack;
            keySlot.destIndex = 10;
            keySlot.depth = 0.3f;
            keySlot.active = true;
            mm.setSlot(1, keySlot);

            // Envelope → dest 10, depth 0.4 (should be SKIPPED in per-voice)
            ModSlot envSlot;
            envSlot.source = ModSource::Envelope;
            envSlot.destIndex = 10;
            envSlot.depth = 0.4f;
            envSlot.active = true;
            mm.setSlot(2, envSlot);

            mm.publish();
            mm.acquire();

            float offset = mm.computePerVoiceOffset(10, 0.8f, 0.5f, 0.0f);
            // velocity 0.8 * 0.5 + keytrack 0.5 * 0.3 = 0.4 + 0.15 = 0.55
            // Envelope is skipped in per-voice
            expect(std::abs(offset - 0.55f) < 0.01f, "Per-voice offset should be 0.55 (got "
                   + juce::String(offset) + ")");
        }

        beginTest("Slot add/remove/clear");
        {
            ModulationMatrix mm;

            // Add a route
            int idx = mm.addRoute(ModSource::LFO2, 7, 0.6f);
            expect(idx >= 0, "addRoute should return valid index");
            mm.publish();
            mm.acquire();
            expect(mm.countActiveRoutes() == 1, "Should have 1 active route");

            // Remove it
            mm.copyReadToWrite();
            mm.removeRoute(ModSource::LFO2, 7);
            mm.publish();
            mm.acquire();
            expect(mm.countActiveRoutes() == 0, "Should have 0 routes after remove");

            // Add multiple, clear all
            mm.addRoute(ModSource::Macro1, 0, 0.3f);
            mm.addRoute(ModSource::Macro2, 1, 0.4f);
            mm.addRoute(ModSource::Macro3, 2, 0.5f);
            mm.publish();
            mm.acquire();
            expect(mm.countActiveRoutes() == 3, "Should have 3 routes");

            mm.clearAllSlots();
            mm.publish();
            mm.acquire();
            expect(mm.countActiveRoutes() == 0, "Should have 0 routes after clearAll");
        }

        beginTest("hasRouteToDest and getRoutesForDest");
        {
            ModulationMatrix mm;

            mm.addRoute(ModSource::Envelope, 5, 0.3f);
            mm.addRoute(ModSource::LFO1, 5, -0.2f);
            mm.addRoute(ModSource::Macro1, 8, 0.7f);
            mm.publish();
            mm.acquire();

            expect(mm.hasRouteToDest(5), "Should have routes to dest 5");
            expect(mm.hasRouteToDest(8), "Should have routes to dest 8");
            expect(!mm.hasRouteToDest(0), "Should NOT have routes to dest 0");

            ModulationMatrix::RouteInfo info[4];
            int count = mm.getRoutesForDest(5, info, 4);
            expect(count == 2, "Should have 2 routes to dest 5 (got " + juce::String(count) + ")");
        }

        beginTest("removeAllRoutesToDest");
        {
            ModulationMatrix mm;

            mm.addRoute(ModSource::Envelope, 5, 0.3f);
            mm.addRoute(ModSource::LFO1, 5, -0.2f);
            mm.addRoute(ModSource::Macro1, 8, 0.7f);
            mm.publish();
            mm.acquire();

            mm.copyReadToWrite();
            mm.removeAllRoutesToDest(5);
            mm.publish();
            mm.acquire();

            expect(!mm.hasRouteToDest(5), "Routes to dest 5 should be gone");
            expect(mm.hasRouteToDest(8), "Route to dest 8 should remain");
            expect(mm.countActiveRoutes() == 1, "Should have 1 remaining route");
        }

        beginTest("Max 32 slots enforced");
        {
            ModulationMatrix mm;

            for (int i = 0; i < MAX_MOD_SLOTS; ++i)
                mm.addRoute(ModSource::Envelope, static_cast<uint8_t>(i % 28), 0.1f);

            int idx = mm.addRoute(ModSource::LFO1, 0, 0.5f);
            expect(idx == -1, "33rd route should fail (returned " + juce::String(idx) + ")");
        }

        beginTest("Inactive slots don't contribute to modulation");
        {
            ModulationMatrix mm;

            ModSlot slot;
            slot.source = ModSource::Envelope;
            slot.destIndex = 0;
            slot.depth = 0.5f;
            slot.active = false; // inactive!
            mm.setSlot(0, slot);
            mm.publish();
            mm.acquire();

            mm.setSourceValue(ModSource::Envelope, 1.0f);
            float result = mm.computeModulatedNorm(0, 0.5f);
            expect(std::abs(result - 0.5f) < 0.001f, "Inactive slot should not modulate (got "
                   + juce::String(result) + ")");
        }
    }
};

static ModulationMatrixTest modMatrixTest;
