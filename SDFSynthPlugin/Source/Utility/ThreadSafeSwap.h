#pragma once
#include <memory>
#include <mutex>

template <typename T>
class ThreadSafeSwap
{
public:
    ThreadSafeSwap() = default;

    void set(std::shared_ptr<T> newData)
    {
        std::lock_guard<std::mutex> lock(mtx);
        data = std::move(newData);
    }

    std::shared_ptr<T> get() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return data;
    }

private:
    mutable std::mutex mtx;
    std::shared_ptr<T> data;
};
