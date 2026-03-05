#include <juce_core/juce_core.h>
#include "Utility/ThreadSafeSwap.h"
#include <thread>
#include <atomic>

class ThreadSafeSwapTests : public juce::UnitTest
{
public:
    ThreadSafeSwapTests() : juce::UnitTest("ThreadSafeSwap") {}

    void runTest() override
    {
        beginTest("Default is null");
        {
            ThreadSafeSwap<int> tss;
            auto ptr = tss.get();
            expect(ptr == nullptr, "Default should be null");
        }

        beginTest("Set and get returns same value");
        {
            ThreadSafeSwap<int> tss;
            tss.set(std::make_shared<int>(42));
            auto ptr = tss.get();
            expect(ptr != nullptr, "Should not be null after set");
            expectEquals(*ptr, 42, "Should return the value that was set");
        }

        beginTest("Set overwrites previous value");
        {
            ThreadSafeSwap<int> tss;
            tss.set(std::make_shared<int>(1));
            tss.set(std::make_shared<int>(2));
            auto ptr = tss.get();
            expectEquals(*ptr, 2, "Should return most recent value");
        }

        beginTest("Get returns shared ownership");
        {
            ThreadSafeSwap<int> tss;
            auto original = std::make_shared<int>(99);
            tss.set(original);
            auto copy = tss.get();
            expect(copy.get() == original.get(), "Should point to same object");
            expect(original.use_count() >= 2, "Ref count should reflect shared ownership");
        }

        beginTest("Concurrent stress test - no crashes or data races");
        {
            ThreadSafeSwap<std::vector<int>> tss;
            tss.set(std::make_shared<std::vector<int>>(100, 0));

            std::atomic<bool> running{ true };
            std::atomic<int> readCount{ 0 };
            std::atomic<int> writeCount{ 0 };

            // Writer thread: rapidly swaps new vectors
            std::thread writer([&]() {
                int val = 0;
                while (running.load(std::memory_order_relaxed))
                {
                    auto v = std::make_shared<std::vector<int>>(100, val);
                    tss.set(v);
                    ++val;
                    writeCount.fetch_add(1, std::memory_order_relaxed);
                }
            });

            // Reader threads: rapidly read and verify consistency
            std::thread reader1([&]() {
                while (running.load(std::memory_order_relaxed))
                {
                    auto ptr = tss.get();
                    if (ptr && !ptr->empty())
                    {
                        int first = (*ptr)[0];
                        for (size_t i = 1; i < ptr->size(); ++i)
                        {
                            if ((*ptr)[i] != first)
                            {
                                // This should never happen — each vector is uniform
                                running.store(false, std::memory_order_relaxed);
                                return;
                            }
                        }
                        readCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });

            std::thread reader2([&]() {
                while (running.load(std::memory_order_relaxed))
                {
                    auto ptr = tss.get();
                    if (ptr && !ptr->empty())
                    {
                        readCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });

            // Run for ~200ms
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            running.store(false, std::memory_order_relaxed);

            writer.join();
            reader1.join();
            reader2.join();

            expect(writeCount.load() > 0, "Writer should have completed some writes");
            expect(readCount.load() > 0, "Readers should have completed some reads");
        }
    }
};

static ThreadSafeSwapTests threadSafeSwapTests;
