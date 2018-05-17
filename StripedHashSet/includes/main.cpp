#include "asserts.h"
#include "barrier.h"
#include "executor.h"
#include "program_options.h"

#include "solution.h"
//#include "hash_set_baseline.h"

#include <algorithm>
#include <random>
#include <thread>

///////////////////////////////////////////////////////////////////////

class ConcurrentSetTester {
public:
    explicit ConcurrentSetTester(const size_t concurrency_level,
                                 const size_t num_inserts,
                                 const size_t num_threads)
        : set_{concurrency_level},
          num_inserts_{num_inserts},
          num_threads_{num_threads},
          barrier_1st_phase_{num_threads},
          barrier_2nd_phase_{num_threads},
          barrier_3rd_phase_{num_threads} {
    }

    void operator ()() {
        RunWriterThreads();
    }

private:
    void RunWriterThreads() {
        TaskExecutor executor{};
        for (size_t thread_index = 0; thread_index < num_threads_; ++thread_index) {
            executor.Run([this, thread_index]() {
                RunWriterThread(thread_index);
            });
        }
    }

    std::vector<int> PrepareWorkingSet(const size_t thread_index) {
        std::vector<int> elements;
        for (size_t i = thread_index; i < num_inserts_; i += num_threads_) {
            elements.push_back(static_cast<int>(i));
        }
        std::random_shuffle(elements.begin(), elements.end());
        return elements;
    }

    void RunWriterThread(const size_t thread_index) {
        std::vector<int> elements{PrepareWorkingSet(thread_index)};

        barrier_1st_phase_.Pass();
        // insert all even numbers
        // concurrent inserts, disjoint working sets

        for (const int e : elements) {
            if (e % 2 == 0) {
                test_assert(!set_.Contains(e), "[phase 1] unexpected element found: " << e);
                test_assert(set_.Insert(e), "[phase 1] insert failed on " << e);
                test_assert(set_.Contains(e), "[phase 1] expected element not found: " << e);
                if (e % 5 == 0) {
                    test_assert(!set_.Insert(e), "[phase 1] duplicated insert: " << e);
                }
            } else {
                test_assert(!set_.Contains(e), "[phase 1] unexpected element found: " << e);
            }
        }

        barrier_2nd_phase_.Pass();
        // remove all even numbers and insert all odd numbers
        // concurrent inserts and removes, disjoint elements

        for (const int e : elements) {
            if (e % 2 == 0) {
                test_assert(set_.Remove(e), "[phase 2] remove failed on " << e);
                test_assert(!set_.Contains(e), "[phase 2] unexpected element found: " << e);
                if (e % 5 == 0) {
                    test_assert(!set_.Remove(e), "[phase 2] duplicated remove: " << e);
                }
            } else {
                test_assert(set_.Insert(e), "[phase 2] insert failed on " << e);
            }
        }

        barrier_3rd_phase_.Pass();
        // validation

        for (const int e : elements) {
            if (e % 2 == 0) {
                test_assert(!set_.Contains(e), "[phase 3] unexpected element found: " << e);
            } else {
                test_assert(set_.Contains(e), "[phase 3] expected element not found: " << e);
            }
        }

        if (thread_index == 0) {
            const size_t odd_numbers_count = (num_inserts_ / 2) + (num_inserts_ % 2);
            test_assert(set_.Size() == odd_numbers_count, "unexpected set size: " << set_.Size() << ", expected: " << odd_numbers_count);
        }
    }

private:
    ConcurrentSet<int> set_;

    size_t num_inserts_;
    size_t num_threads_;

    OnePassBarrier barrier_1st_phase_;
    OnePassBarrier barrier_2nd_phase_;
    OnePassBarrier barrier_3rd_phase_;
};

///////////////////////////////////////////////////////////////////////

void RunTest(int argc, char* argv[]) {
    size_t concurrency_level;
    size_t num_inserts;
    size_t num_threads;

    read_opts(argc, argv, concurrency_level, num_inserts, num_threads);

    ConcurrentSetTester{concurrency_level, num_inserts, num_threads}();
}

int main(int argc, char* argv[]) {
    RunTest(argc, argv);
    return EXIT_SUCCESS;
}
