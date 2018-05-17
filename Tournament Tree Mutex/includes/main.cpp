#include "solution.h"
#include "test_utils.h"

#include <atomic>
#include <string>
#include <sstream>
#include <vector>
#include <thread>

namespace SolutionTests {

#if defined(dump_field)
#   error "already defined"
#else
#   define dump_field(field) out << (#field) << "=" << (field)
#endif

    struct TestOpts {
        size_t n_threads = 0;
        size_t n_iters = 0;
        size_t n_shareds = 0;

        TestOpts(int argc, char* argv[]) {
            read_opts(argc, argv, n_threads, n_iters, n_shareds);
        }

        std::string to_string() const {
            std::ostringstream out;
            dump_field(n_threads) << ", ";
            dump_field(n_iters) << ", ";
            dump_field(n_shareds);
            return out.str();
        }
    };


    template <class Mutex>
    class MutexTester {
        Mutex mutex;
        std::atomic<bool> already_inside_critical{std::memory_order_relaxed};

    public:
        explicit MutexTester(size_t nthr)
            : mutex(nthr)
        {}

        void lock(size_t thr) {
            mutex.lock(thr);
            test_assert(!already_inside_critical.exchange(true), "mutual exclusion");
        }

        void unlock(size_t thr) {
            test_assert(already_inside_critical.exchange(false), "mutual exclusion");
            mutex.unlock(thr);
        }
    };

    void do_test(const TestOpts& opts) {
        std::cout << "Testing with parameters: " << opts.to_string() << std::endl;

        std::vector<size_t> shared_vars(opts.n_shareds);
        volatile size_t tmp = 0;

        MutexTester<TreeMutex> mutex(opts.n_threads);
        Barrier barrier_start(opts.n_threads);
        Barrier barrier_end(opts.n_threads);
        std::vector<std::thread> threads(opts.n_threads);

        std::cout << "Starting " << opts.n_threads << " threads ... " << std::endl;

        for (size_t thr = 0; thr < opts.n_threads; ++thr) {

            threads[thr] = std::thread([thr, &tmp, &shared_vars, &opts, &mutex, &barrier_start, &barrier_end]() {
                barrier_start.wait();

                for (size_t i = 0; i < opts.n_iters; ++i) {
                    mutex.lock(thr);

                    for (auto& shared : shared_vars) {
                        tmp = shared;
                        tmp += 1;
                        std::this_thread::yield();
                        shared = tmp;
                    }

                    mutex.unlock(thr);
                }

                barrier_end.wait();
            });
        }

        std::cout << "Done" << std::endl;
        std::cout << "Joining " << opts.n_threads << " threads ... " << std::endl;

        for (auto& thr : threads) {
            if (thr.joinable()) {
                thr.join();
            }
        }

        std::cout << "Done" << std::endl;
        std::cout << "Checking test invariant ... " << std::endl;

        const size_t expected = opts.n_iters * opts.n_threads;
        for (const auto& shared_var : shared_vars) {
            test_assert(shared_var == expected,
                        "mutual exclusion property failed for " << opts.n_threads << " threads: " << shared_var << " != " << expected);
        }

        std::cout << "Done" << std::endl;
        std::cout << "OK" << std::endl;
    }

    void run_tests(int argc, char* argv[]) {
        do_test(TestOpts{argc, argv});
    }
}

int main(int argc, char* argv[]) {
    SolutionTests::run_tests(argc, argv);
}
