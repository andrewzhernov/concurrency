#pragma once

#include <condition_variable>
#include <iostream>
#include <mutex>

class Semaphore {
 public:
  Semaphore(size_t init_value = 0)
      : state_{init_value} {
  }

  void Signal(const size_t count = 1) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ += count;
    semaphore_cv_.notify_one();
  }

  void Wait(const size_t count = 1) {
    std::unique_lock<std::mutex> lock(mutex_);
    semaphore_cv_.wait(lock, [&] { return state_ >= count; });
    state_ -= count;
  }

 private:
  size_t state_;
  std::mutex mutex_;
  std::condition_variable semaphore_cv_;
};

class Robot {
 public:
  Robot()
      : left_{1}
      , right_{0} {
  }

  void StepLeft() {
    left_.Wait();
    std::cout << "left" << std::endl;
    right_.Signal();
  }

  void StepRight() {
    right_.Wait();
    std::cout << "right" << std::endl;
    left_.Signal();
  }

 private:
  Semaphore left_;
  Semaphore right_;
};
