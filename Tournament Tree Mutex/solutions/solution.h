#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

class PetersonMutex {
public:
    PetersonMutex() {
        want_[0].store(false);
        want_[1].store(false);
        victim_.store(0);
    }

    void lock(int index) {
        want_[index].store(true);
        victim_.store(index);
        while (want_[1 - index].load() && victim_.load() == index) {
            std::this_thread::yield();
        }
    }

    void unlock(int index) {
        want_[index].store(false);
    }

private:
    std::array<std::atomic<bool>, 2> want_;
    std::atomic<int> victim_;

    PetersonMutex(const PetersonMutex& mutex);
};

class TreeMutex {
public:
    TreeMutex(std::size_t n_threads)
        : height_(0)
    {
        std::size_t size = 1;
        while (size < n_threads) {
             size *= 2;
             ++height_;
        }
        mutexes_ = std::vector<PetersonMutex>(size);
    }

    void lock(std::size_t current_thread) {
        for (std::size_t current = mutexes_.size() + current_thread;
             current > 1;
             current /= 2) {
                mutexes_[current / 2].lock(current % 2);
        }
    }

    void unlock(std::size_t current_thread) {
        std::size_t way = InvertLastBits(mutexes_.size() + current_thread);
        std::size_t parent = 1;
        for (; way > 1; way /= 2) {
            mutexes_[parent].unlock(way % 2);
            parent = parent * 2 + way % 2;
        }
    }

private:
    std::size_t height_;
    std::vector<PetersonMutex> mutexes_;

    std::size_t InvertLastBits(std::size_t number) {
        std::size_t result = 1;
        for (std::size_t i = 0; i < height_; ++i) {
            result = result * 2 + number % 2;
            number /= 2;
        }
        return result;
    }
};
