#pragma once

#include "arena_allocator.h"

#include <atomic>
#include <limits>
#include <mutex>
#include <thread>

///////////////////////////////////////////////////////////////////////

template <typename T>
struct ElementTraits {
  static T Min() {
    return std::numeric_limits<T>::min();
  }
  static T Max() {
    return std::numeric_limits<T>::max();
  }
};

///////////////////////////////////////////////////////////////////////

class SpinLock {
 public:
  explicit SpinLock()
      : owner_ticket{0}
      , next_ticket{0} {
  }

  void Lock() {
    size_t current_ticket = next_ticket.fetch_add(1);
    while (current_ticket != owner_ticket) {
      std::this_thread::yield();
    }
  }

  void Unlock() {
    ++owner_ticket;
  }

  // adapters for BasicLockable concept
  void lock() {
    Lock();
  }

  void unlock() {
    Unlock();
  }

 private:
  std::atomic<size_t> owner_ticket;
  std::atomic<size_t> next_ticket;
};

///////////////////////////////////////////////////////////////////////

template <typename T>
class OptimisticLinkedSet {
 private:
  struct Node {
    T element_;
    std::atomic<Node*> next_;
    SpinLock lock_;
    std::atomic<bool> marked_{false};

    Node(const T& element, Node* next = nullptr)
      : element_(element),
        next_(next) {
    }
  };

  struct Edge {
    Node* pred_;
    Node* curr_;

    Edge(Node* pred, Node* curr)
      : pred_(pred),
        curr_(curr) {
    }
  };

 public:
  explicit OptimisticLinkedSet(ArenaAllocator& allocator)
      : allocator_(allocator) {
    CreateEmptyList();
  }

  bool Insert(const T& element) {
    bool valid = false;
    do {
      Edge edge{Locate(element)};
      if (element == edge.curr_->element_) return false;
      std::lock_guard<SpinLock> pred_lock{edge.pred_->lock_};
      std::lock_guard<SpinLock> curr_lock{edge.curr_->lock_};
      valid = Validate(edge);
      if (valid) {
        edge.pred_->next_ = allocator_.New<Node>(element, edge.pred_->next_);
        ++size_;
        return true;
      }
    } while (!valid);
    return false;
  }

  bool Remove(const T& element) {
    bool valid = false;
    do {
      Edge edge{Locate(element)};
      std::lock_guard<SpinLock> pred_lock{edge.pred_->lock_};
      std::lock_guard<SpinLock> curr_lock{edge.curr_->lock_};
      valid = Validate(edge);
      if (valid) {
	if (edge.curr_->element_ != element) return false;
        edge.curr_->marked_ = true;
        edge.pred_->next_.store(edge.curr_->next_);
        --size_;
        return true;
      }
    } while (!valid);
    return false;
  }

  bool Contains(const T& element) const {
    Edge edge{Locate(element)};
    return !edge.curr_->marked_ && edge.curr_->element_ == element;
  }

  size_t Size() const {
    return size_;
  }

 private:
  void CreateEmptyList() {
    head_ = allocator_.New<Node>(ElementTraits<T>::Min());
    head_->next_ = allocator_.New<Node>(ElementTraits<T>::Max());
  }

  Edge Locate(const T& element) const {
    Node* pred = head_;
    Node* curr = head_->next_;
    while (!(pred->element_ < element && element <= curr->element_)) {
      pred = curr;
      curr = curr->next_;
    }
    return std::move(Edge(pred, curr));
  }

  bool Validate(const Edge& edge) const {
    return !edge.pred_->marked_ && !edge.curr_->marked_ &&
            edge.pred_->next_ == edge.curr_;
  }

 private:
  ArenaAllocator& allocator_;
  Node* head_{nullptr};
  std::atomic<size_t> size_{0};
};

template <typename T> using ConcurrentSet = OptimisticLinkedSet<T>;

///////////////////////////////////////////////////////////////////////
