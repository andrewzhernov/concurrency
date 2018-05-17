#pragma once

#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

template <class T>
class ThreadPool {
public:
    ThreadPool(size_t num_threads = DefaultNumWorkers())
        : done_(false)
    {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back(std::thread(&ThreadPool::WorkerThread, this));
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    std::future<T> Submit(std::function<T()> task) {
        std::packaged_task<T()> packaged_task(std::move(task));
        std::future<T> result(packaged_task.get_future());
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (done_) {
                throw std::exception();
            }
            task_queue_.push_back(std::move(packaged_task));
        }
        condition_.notify_one();
        return result;
    }

    void Shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (done_) return;
            done_ = true;
        }
        condition_.notify_all();
        for (auto& worker: workers_) {
            worker.join();
        }
    }

    ~ThreadPool() {
        Shutdown();
    }

private:
    bool done_;
    std::mutex mtx_;
    std::condition_variable condition_;
    std::deque<std::packaged_task<T()>> task_queue_;
    std::vector<std::thread> workers_;

    void WorkerThread() {
        for (;;) {
            std::packaged_task<T()> task;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                condition_.wait(lock, [this] {
                    return !task_queue_.empty() || done_;
                });
                if (done_ && task_queue_.empty()) {
                    return;
                }
                task = std::move(task_queue_.front());
                task_queue_.pop_front();
            }
            task();
        }
    }

    static size_t DefaultNumWorkers() {
        size_t num_threads = std::thread::hardware_concurrency();
        return (num_threads ? num_threads : 4);
    }
};
