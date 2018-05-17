#include "solution.h"
#include "test_utils.h"

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

        TestOpts(int argc, char* argv[]) {
            read_opts(argc, argv, n_threads, n_iters);
        }

        std::string to_string() const {
            std::ostringstream out;
            dump_field(n_threads) << ", ";
            dump_field(n_iters);
            return out.str();
        }
    };


    template <class CyclicBarrier>
    class CyclicBarrierTester {
        CyclicBarrier barrier;
        std::vector<size_t> values;
    public:
        explicit CyclicBarrierTester(size_t nthr)
            : barrier(nthr), values(nthr, 0)
        {}

        void set_value(size_t thr, size_t value) {
            values[thr] = value;
        }

        void assert_values(size_t expected) {
            for (size_t v : values) {
                test_assert(v == expected, "barrier failed to synchronize all threads on iteration " << expected);
            }
        }

        void pass() {
            barrier.Pass();
        }
    };

    void do_test(const TestOpts& opts) {
        std::cout << "Testing with parameters: " << opts.to_string() << std::endl;

        CyclicBarrierTester<CyclicBarrier<ConditionVariableWithSpuriousWakeups>> barrier(opts.n_threads);
        Barrier barrier_start(opts.n_threads);
        Barrier barrier_end(opts.n_threads);
        std::vector<std::thread> threads(opts.n_threads);

        std::cout << "Starting " << opts.n_threads << " threads ... " << std::endl;

        for (size_t thr = 0; thr < opts.n_threads; ++thr) {
            threads[thr] = std::thread([thr, &opts, &barrier, &barrier_start, &barrier_end]() {
                barrier_start.wait();

                for (size_t i = 0; i < opts.n_iters; ++i) {
                    barrier.set_value(thr, i);
                    barrier.pass();
                    barrier.assert_values(i);
                    barrier.pass();
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

        barrier.assert_values(opts.n_iters - 1);

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
