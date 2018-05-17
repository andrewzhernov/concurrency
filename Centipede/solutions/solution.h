#pragma once

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

class Semaphore {
 public:
  explicit Semaphore(const size_t init_value = 0)
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
  Robot(const size_t num_feet) {
    centipede_.reserve(num_feet);
    for (size_t i = 0; i < num_feet; ++i) {
      centipede_.emplace_back(std::make_unique<Semaphore>());
    }
    if (num_feet) centipede_.front()->Signal();
  }

  void Step(const size_t foot) {
    if (foot < centipede_.size()) {
      centipede_[foot]->Wait();
      std::cout << "foot " << foot << std::endl;
      centipede_[(foot + 1) % centipede_.size()]->Signal();
    }
  }

 private:
  std::vector<std::unique_ptr<Semaphore>> centipede_;
};
