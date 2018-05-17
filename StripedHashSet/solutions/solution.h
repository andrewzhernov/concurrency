#pragma once

#include <atomic>
#include <algorithm>
#include <condition_variable>
#include <exception>
#include <forward_list>
#include <functional>
#include <mutex>
#include <vector>

///////////////////////////////////////////////////////////////////////

class ReadWriteMutex {
 public:
  void ReadLock() {
    std::unique_lock<std::mutex> lock(mutex_);
    read_cv_.wait(lock, [this] { return !writers_; });
    ++readers_;
  }

  void ReadUnlock() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (readers_) {
      --readers_;
      if (!readers_) write_cv_.notify_one();
    }
  }

  void WriteLock() {
    std::unique_lock<std::mutex> lock(mutex_);
    ++writers_;
    write_cv_.wait(lock, [this] { return !readers_ && !writing_; });
    writing_ = true;
  }

  void WriteUnlock() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (writers_) {
      --writers_;
      writing_ = false;
      if (writers_) {
        write_cv_.notify_one();
      } else {
        read_cv_.notify_all();
      }
    }
  }

 private:
  size_t readers_;
  size_t writers_;
  bool writing_;
  std::mutex mutex_;
  std::condition_variable read_cv_;
  std::condition_variable write_cv_;
};

///////////////////////////////////////////////////////////////////////

class Locker {
 public:
  enum class MODE {READ, WRITE};

  explicit Locker(ReadWriteMutex& mutex, MODE mode)
        : locked_{true}
        , mode_{mode}
        , mutex_{mutex} {
    if (mode_ == MODE::WRITE) {
      mutex_.WriteLock();
    } else {
      mutex_.ReadLock();
    }
  }

  void Unlock() {
    if (locked_) {
      locked_ = false;
      if (mode_ == MODE::WRITE) {
        mutex_.WriteUnlock();
      } else {
        mutex_.ReadUnlock();
      }
    }
  }

  ~Locker() {
    Unlock();
  }

 private:
  bool locked_;
  MODE mode_;
  ReadWriteMutex& mutex_;
};

///////////////////////////////////////////////////////////////////////

template<typename T, class Hash = std::hash<T>>
class StripedHashSet {
 public:
  explicit StripedHashSet(const size_t concurrency_level,
                          const size_t growth_factor = 3,
                          const double load_factor = 0.75)
      : size_{0}
      , growth_factor_{growth_factor}
      , load_factor_{load_factor}
      , blocked_{false}
      , buckets_{DefaultNumBuckets(concurrency_level)}
      , stripes_{concurrency_level} {
  }

  bool Insert(const T& element) {
    size_t hash_value = HashFunction(element);
    Locker locker(stripes_[GetStripeIndex(hash_value)], Locker::MODE::WRITE);
    Bucket& bucket = buckets_[GetBucketIndex(hash_value)];

    if (!Find(bucket, element)) {
      bucket.push_front(element);
      ++size_;
      if (load_factor_ * (double)buckets_.size() < (double)size_ &&
            !blocked_.exchange(true)) {
        locker.Unlock();
        Reallocate();
      }
      return true;
    }
    return false;
  }

  bool Remove(const T& element) {
    size_t hash_value = HashFunction(element);
    Locker locker(stripes_[GetStripeIndex(hash_value)], Locker::MODE::WRITE);
    Bucket& bucket = buckets_[GetBucketIndex(hash_value)];

    if (Find(bucket, element)) {
      bucket.remove(element);
      --size_;
      return true;
    }
    return false;
  }

  bool Contains(const T& element) const {
    size_t hash_value = HashFunction(element);
    Locker locker(stripes_[GetStripeIndex(hash_value)], Locker::MODE::READ);
    const Bucket& bucket = buckets_[GetBucketIndex(hash_value)];

    return Find(bucket, element);
  }

  size_t Size() const {
    return size_;
  }

 private:
  using Bucket = std::forward_list<T>;
  using HashSet = std::vector<Bucket>;

  Hash HashFunction;

  size_t GetBucketIndex(const size_t hash_value) const {
    return hash_value % buckets_.size();
  }

  size_t GetStripeIndex(const size_t hash_value) const {
    return hash_value % stripes_.size();
  }

  bool Find(const Bucket& bucket, const T& element) const {
    return std::find(bucket.begin(), bucket.end(), element) != bucket.end();
  }

  void Reallocate() {
    for (auto& mutex : stripes_) {
      mutex.WriteLock();
    }
    HashSet old_buckets = HashSet{growth_factor_ * buckets_.size()};
    buckets_.swap(old_buckets);
    for (auto& bucket : old_buckets) {
      for (auto& element : bucket) {
        size_t index = GetBucketIndex(HashFunction(element));
        buckets_[index].push_front(element);
      }
    }
    blocked_ = false;
    for (auto& mutex : stripes_) {
      mutex.WriteUnlock();
    }
  }

  static size_t DefaultNumBuckets(const size_t concurrency_level) {
    if (!concurrency_level) throw std::exception();
    const size_t DEFAULT_NUM_BUCKETS = 30;
    return concurrency_level * (DEFAULT_NUM_BUCKETS / concurrency_level + 1);
  }

  std::atomic<size_t> size_;
  size_t growth_factor_;
  double load_factor_;

  std::atomic<bool> blocked_;
  HashSet buckets_;
  mutable std::vector<ReadWriteMutex> stripes_;
};

template<typename T> using ConcurrentSet = StripedHashSet<T>;

///////////////////////////////////////////////////////////////////////
