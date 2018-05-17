#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>

template <typename ConditionVariable = std::condition_variable>
class CyclicBarrier {
public:
    CyclicBarrier(std::size_t num_threads)
        : num_threads_(num_threads)
        , entered_(false)
        , exited_(false)
        , threads_waiting_(0)
    {}

    void Pass() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (++threads_waiting_ == num_threads_) {
            entered_ = true;
            exited_ = false;
            cv_.notify_all();
        } else {
            cv_.wait(lock, [&] { return entered_; });
        }

        if (--threads_waiting_ == 0) {
            entered_ = false;
            exited_ = true;
            cv_.notify_all();
        } else {
            cv_.wait(lock, [&] { return exited_; });
        }
    }

private:
    std::size_t num_threads_;
    bool entered_;
    bool exited_;
    std::size_t threads_waiting_;
    std::mutex mutex_;
    ConditionVariable cv_;
};
