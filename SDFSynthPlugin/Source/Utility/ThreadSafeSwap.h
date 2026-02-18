#pragma once
#include <atomic>
#include <memory>

template <typename T>
class ThreadSafeSwap
{
public:
    ThreadSafeSwap() = default;

    void set(std::shared_ptr<T> newData)
    {
        std::atomic_store(&data, newData);
    }

    std::shared_ptr<T> get() const
    {
        return std::atomic_load(&data);
    }

private:
    std::shared_ptr<T> data;
};
