#pragma once

#include <condition_variable>
#include <mutex>

///////////////////////////////////////////////////////////////////////

class OnePassBarrier {
public:
    explicit OnePassBarrier(const size_t num_threads)
        : thread_count_{num_threads} {
    }

    void Pass() {
        std::unique_lock<std::mutex> lock{mutex_};
        --thread_count_;
        if (thread_count_ == 0) {
            all_threads_arrived_.notify_all();
        } else {
            all_threads_arrived_.wait(lock, [this]() { return thread_count_ == 0; });
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable all_threads_arrived_;
    size_t thread_count_;
};

///////////////////////////////////////////////////////////////////////
