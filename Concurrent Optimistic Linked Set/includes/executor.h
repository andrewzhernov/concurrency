#pragma once

#include <functional>
#include <thread>
#include <vector>

/////////////////////////////////////////////////////////////////////

class TaskExecutor {
public:
    void Run(std::function<void()> task) {
        worker_threads_.emplace_back(task);
    }

    ~TaskExecutor() {
        for (auto& worker : worker_threads_) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> worker_threads_;
};

/////////////////////////////////////////////////////////////////////
