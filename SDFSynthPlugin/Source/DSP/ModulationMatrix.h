#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <algorithm>
#include <cmath>

enum class ModSource : uint8_t
{
    Envelope = 0,
    LFO1,
    LFO2,
    ModWheel,
    Velocity,
    Aftertouch,
    KeyTrack,
    Random,
    Macro1,
    Macro2,
    Macro3,
    Macro4,
    Count // = 12
};

static constexpr int MOD_SOURCE_COUNT = static_cast<int>(ModSource::Count);

struct ModSlot
{
    ModSource source = ModSource::Envelope;
    uint8_t destIndex = 0;     // index into modulatable param ID list
    float depth = 0.f;         // bipolar -1..+1
    bool active = false;
};

static constexpr int MAX_MOD_SLOTS = 32;

class ModulationMatrix
{
public:
    ModulationMatrix()
    {
        for (auto& b : buffers)
            b.slots.fill(ModSlot{});
        sourceValues.fill(0.f);
    }

    // =====================================================================
    // GUI thread: build routes, then publish
    // =====================================================================

    void setSlot(int index, ModSlot slot)
    {
        if (index < 0 || index >= MAX_MOD_SLOTS) return;
        buffers[writeIdx.load(std::memory_order_relaxed)].slots[static_cast<size_t>(index)] = slot;
    }

    void clearSlot(int index)
    {
        if (index < 0 || index >= MAX_MOD_SLOTS) return;
        buffers[writeIdx.load(std::memory_order_relaxed)].slots[static_cast<size_t>(index)] = ModSlot{};
    }

    void clearAllSlots()
    {
        auto& buf = buffers[writeIdx.load(std::memory_order_relaxed)];
        buf.slots.fill(ModSlot{});
    }

    // Copy read buffer to write buffer so GUI edits are based on current state
    void copyReadToWrite()
    {
        int r = readIdx.load(std::memory_order_acquire);
        int w = writeIdx.load(std::memory_order_relaxed);
        buffers[w] = buffers[r];
    }

    const ModSlot& getWriteSlot(int index) const
    {
        return buffers[writeIdx.load(std::memory_order_relaxed)].slots[static_cast<size_t>(
            std::clamp(index, 0, MAX_MOD_SLOTS - 1))];
    }

    // Atomic swap write→pending
    void publish()
    {
        int w = writeIdx.load(std::memory_order_relaxed);
        int p = pendingIdx.load(std::memory_order_acquire);
        // Copy write buffer to pending slot
        buffers[p] = buffers[w];
        // Swap pending and write indices
        pendingIdx.store(w, std::memory_order_release);
        writeIdx.store(p, std::memory_order_relaxed);
        dirty.store(true, std::memory_order_release);
    }

    // =====================================================================
    // Audio thread: acquire latest snapshot
    // =====================================================================

    void acquire()
    {
        if (dirty.load(std::memory_order_acquire))
        {
            int p = pendingIdx.load(std::memory_order_acquire);
            int r = readIdx.load(std::memory_order_relaxed);
            pendingIdx.store(r, std::memory_order_release);
            readIdx.store(p, std::memory_order_relaxed);
            dirty.store(false, std::memory_order_release);
        }
    }

    const std::array<ModSlot, MAX_MOD_SLOTS>& getSlots() const
    {
        return buffers[readIdx.load(std::memory_order_acquire)].slots;
    }

    // =====================================================================
    // Source value storage (written by audio thread each block)
    // =====================================================================

    void setSourceValue(ModSource src, float value)
    {
        int idx = static_cast<int>(src);
        if (idx >= 0 && idx < MOD_SOURCE_COUNT)
            sourceValues[static_cast<size_t>(idx)] = value;
    }

    float getSourceValue(ModSource src) const
    {
        int idx = static_cast<int>(src);
        if (idx >= 0 && idx < MOD_SOURCE_COUNT)
            return sourceValues[static_cast<size_t>(idx)];
        return 0.f;
    }

    // =====================================================================
    // Compute modulated normalized value for a destination
    // Sums all active routes targeting destIndex, clamps to [0,1]
    // =====================================================================

    float computeModulatedNorm(int destIndex, float baseNorm) const
    {
        float offset = 0.f;
        auto& slots = getSlots();
        for (auto& slot : slots)
        {
            if (!slot.active) continue;
            if (slot.destIndex != static_cast<uint8_t>(destIndex)) continue;

            float srcVal = getSourceValue(slot.source);
            offset += srcVal * slot.depth;
        }
        return std::clamp(baseNorm + offset, 0.f, 1.f);
    }

    // =====================================================================
    // Per-voice: compute offset for per-voice sources only
    // (Velocity, KeyTrack, Random — not stored in global sourceValues)
    // =====================================================================

    float computePerVoiceOffset(int destIndex, float velocity, float keytrack, float random) const
    {
        float offset = 0.f;
        auto& slots = getSlots();
        for (auto& slot : slots)
        {
            if (!slot.active) continue;
            if (slot.destIndex != static_cast<uint8_t>(destIndex)) continue;

            float srcVal = 0.f;
            switch (slot.source)
            {
                case ModSource::Velocity:   srcVal = velocity;  break;
                case ModSource::KeyTrack:   srcVal = keytrack;  break;
                case ModSource::Random:     srcVal = random;    break;
                default: continue; // skip global sources
            }
            offset += srcVal * slot.depth;
        }
        return offset;
    }

    // =====================================================================
    // Utility: find first free slot, add route, return slot index or -1
    // =====================================================================

    int addRoute(ModSource source, uint8_t destIndex, float depth)
    {
        int w = writeIdx.load(std::memory_order_relaxed);
        auto& slots = buffers[w].slots;
        for (int i = 0; i < MAX_MOD_SLOTS; ++i)
        {
            if (!slots[static_cast<size_t>(i)].active)
            {
                slots[static_cast<size_t>(i)] = { source, destIndex, depth, true };
                return i;
            }
        }
        return -1;
    }

    // Remove first route matching source+dest
    void removeRoute(ModSource source, uint8_t destIndex)
    {
        int w = writeIdx.load(std::memory_order_relaxed);
        auto& slots = buffers[w].slots;
        for (auto& slot : slots)
        {
            if (slot.active && slot.source == source && slot.destIndex == destIndex)
            {
                slot = ModSlot{};
                return;
            }
        }
    }

    // Remove all routes to a destination
    void removeAllRoutesToDest(uint8_t destIndex)
    {
        int w = writeIdx.load(std::memory_order_relaxed);
        auto& slots = buffers[w].slots;
        for (auto& slot : slots)
        {
            if (slot.active && slot.destIndex == destIndex)
                slot = ModSlot{};
        }
    }

    // Count active routes
    int countActiveRoutes() const
    {
        int count = 0;
        auto& slots = getSlots();
        for (auto& slot : slots)
            if (slot.active) ++count;
        return count;
    }

    // Check if any route targets a destination
    bool hasRouteToDest(int destIndex) const
    {
        auto& slots = getSlots();
        for (auto& slot : slots)
            if (slot.active && slot.destIndex == static_cast<uint8_t>(destIndex))
                return true;
        return false;
    }

    // Get all routes targeting a destination (for GUI display)
    struct RouteInfo { ModSource source; float depth; int slotIndex; };

    int getRoutesForDest(int destIndex, RouteInfo* out, int maxOut) const
    {
        int count = 0;
        auto& slots = getSlots();
        for (int i = 0; i < MAX_MOD_SLOTS && count < maxOut; ++i)
        {
            auto& slot = slots[static_cast<size_t>(i)];
            if (slot.active && slot.destIndex == static_cast<uint8_t>(destIndex))
            {
                out[count++] = { slot.source, slot.depth, i };
            }
        }
        return count;
    }

private:
    struct Buffer
    {
        std::array<ModSlot, MAX_MOD_SLOTS> slots{};
    };

    Buffer buffers[3];
    std::atomic<int> writeIdx{ 0 };
    std::atomic<int> readIdx{ 1 };
    std::atomic<int> pendingIdx{ 2 };
    std::atomic<bool> dirty{ false };
    std::array<float, MOD_SOURCE_COUNT> sourceValues{};
};
