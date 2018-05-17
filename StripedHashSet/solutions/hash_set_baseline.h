#pragma once

#include <shared_mutex>
#include <mutex>
#include <unordered_set>

///////////////////////////////////////////////////////////////////////

template <typename T>
class ConcurrentSet {
 private:
    using SharedMutex = std::shared_timed_mutex;
    using WriterLock = std::unique_lock<SharedMutex>;
    using ReaderLock = std::shared_lock<SharedMutex>;

 public:
    explicit ConcurrentSet(const size_t /* concurrency_level */) {
    }

    bool Insert(const T& element) {
        WriterLock writer_lock{mutex_};
        const auto result = set_.insert(element);
        return result.second;
    }

    bool Remove(const T& element) {
        WriterLock writer_lock{mutex_};
        const size_t count = set_.erase(element);
        return count > 0;
    }

    bool Contains(const T& element) const {
        ReaderLock reader_lock{mutex_};
        return set_.find(element) != set_.end();
    }

    size_t Size() const {
        ReaderLock reader_lock{mutex_};
        return set_.size();
    }

 private:
    std::unordered_set<T> set_;
    mutable SharedMutex mutex_;
};

///////////////////////////////////////////////////////////////////////
