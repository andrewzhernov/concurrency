#pragma once

#include "arena_allocator.h"

#include <algorithm>
#include <list>
#include <mutex>
#include <shared_mutex>

///////////////////////////////////////////////////////////////////////

template <typename T>
class ConcurrentSet {
private:
    using SharedMutex = std::shared_timed_mutex;
    using WriterLock = std::unique_lock<SharedMutex>;
    using ReaderLock = std::shared_lock<SharedMutex>;

public:
    explicit ConcurrentSet(ArenaAllocator& /* allocator */) {
    }

    bool Insert(const T& element) {
        WriterLock writer_lock{mutex_};

        auto pos = std::lower_bound(list_.begin(), list_.end(), element);
        if (pos == list_.end()) {
            list_.push_back(element);
            return true;
        } else if (*pos != element) {
            list_.insert(pos, element);
            return true;
        } else {
            return false;
        }
    }

    bool Remove(const T& element) {
        WriterLock writer_lock{mutex_};

        auto pos = std::find(list_.begin(), list_.end(), element);
        if (pos != list_.end()) {
            list_.erase(pos);
            return true;
        } else {
            return false;
        }
    }

    bool Contains(const T& element) const {
        ReaderLock reader_lock{mutex_};
        return std::find(list_.begin(), list_.end(), element) != list_.end();
    }

    size_t Size() const {
        ReaderLock reader_lock{mutex_};
        return list_.size();
    }

private:
    std::list<T> list_;
    mutable SharedMutex mutex_;
};

///////////////////////////////////////////////////////////////////////
