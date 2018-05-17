#include <iostream>
#include <condition_variable>
#include <queue>
#include <mutex>
#include <stdexcept>

template <class T, class Container = std::deque<T>>
class BlockingQueue {
public:
  explicit BlockingQueue(const size_t capacity)
      : blocked_{false}
      , capacity_{capacity} {
  }

  void Put(T&& item) {
    if (blocked_) {
      throw std::exception();
    }
    std::unique_lock<std::mutex> lock(mtx_);
    if (data_queue_.size() == capacity_) {
      write_cond_.wait(lock, [&] {
        return data_queue_.size() < capacity_ || blocked_;
      });
      if (blocked_) {
        throw std::exception();
      }
    }
    data_queue_.push_back(std::move(item));
    read_cond_.notify_one();
  }

  bool Get(T& result) {
    if (blocked_) {
      return false;
    }
    std::unique_lock<std::mutex> lock(mtx_);
    if (data_queue_.empty()) {
      read_cond_.wait(lock, [&] {
        return !data_queue_.empty() || blocked_;
      });
      if (blocked_) {
        return false;
      }
    }
    result = std::move(data_queue_.front());
    data_queue_.pop_front();
    write_cond_.notify_one();
    return true;
  }

  void Shutdown() {
     std::lock_guard<std::mutex> lock(mtx_);
     blocked_ = true;
     read_cond_.notify_all();
     write_cond_.notify_all();
  }

private:
  size_t capacity_;
  bool blocked_;
  std::mutex mtx_;
  Container data_queue_;
  std::condition_variable read_cond_;
  std::condition_variable write_cond_;
};
