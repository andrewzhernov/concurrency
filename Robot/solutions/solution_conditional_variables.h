#pragma once

#include <condition_variable>
#include <iostream>
#include <mutex>

class Robot {
public:
    Robot()
        : foot_{false} {
    }

    void StepLeft() {
        std::unique_lock<std::mutex> lock(mutex_);
        left_cv_.wait(lock, [this] { return !foot_; });
        std::cout << "left" << std::endl;
        foot_ = true;
        right_cv_.notify_one();
   }

   void StepRight() {
        std::unique_lock<std::mutex> lock(mutex_);
        right_cv_.wait(lock, [this] { return foot_; });
        std::cout << "right" << std::endl;
        foot_ = false;
        left_cv_.notify_one();
    }

private:
    bool foot_;    // left -- false, right -- true
    std::mutex mutex_;
    std::condition_variable left_cv_;
    std::condition_variable right_cv_;
};
