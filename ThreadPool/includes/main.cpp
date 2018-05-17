#include <algorithm>
#include <atomic>
#include <deque>
#include <string>
#include <vector>
#include <sstream>

#include "test_utils.h"
#include "solution.h"
//#include "ya_contest_sim.h"

namespace PoolTests {

#if defined(dump_field)
#   error "already defined"
#else
#   define dump_field(field) out << (#field) << "=" << (field)
#endif

    struct TestPoolOpts {
        size_t n_workers = 10;
        size_t n_items = 100000;

        std::string to_string() const {
            std::ostringstream out;
            dump_field(n_workers) << ", ";
            dump_field(n_items);
            return out.str();
        }
    };

    using Value = std::pair<size_t, std::string>;
    class ValueException : public std::exception {
        Value result;
    public:
        ValueException(const Value& res)
            : result(res)
        {}

        Value get_result() const {
            return result;
        }
    };

    void validate_results(std::vector<Value>& results) {
        std::sort(results.begin(), results.end());
        size_t first = 0;
        for (const auto& result : results) {
            test_assert(result.second == std::to_string(result.first), "invalid result");
            test_assert(first == result.first, "invalid result");
            first++;
        }
    }

    void check_results(std::vector<std::future<Value>>& futures) {
        std::vector<Value> results;
        for (auto& f_item : futures) {
            auto item = f_item.get();
            results.push_back(item);
        }
        validate_results(results);
    }

    void check_exceptions(std::vector<std::future<Value>>& futures) {
        std::vector<Value> results;
        for (auto& f_item : futures) {
            try {
                auto item = f_item.get();
            } catch (const ValueException& e) {
                results.push_back(e.get_result());
            }
        }
        test_assert(results.size() == futures.size(), "invalid result");
        validate_results(results);
    }

    struct count_guard {
        std::atomic_size_t& count;
        const size_t max_concurrency;
        count_guard(std::atomic_size_t& cnt, size_t max_conc)
            : count(cnt)
            , max_concurrency(max_conc)
        {
            size_t n = count.fetch_add(1, std::memory_order_relaxed);
            test_assert(n < max_concurrency,
                        "concurrency exceeded number of workers: " << n + 1 << " > " << max_concurrency);
        }

        ~count_guard() {
            size_t n = count.fetch_sub(1, std::memory_order_relaxed);
            test_assert(n <= max_concurrency,
                        "concurrency exceeded number of workers: " << n << " > " << max_concurrency);
        }
    };

    void do_test_pool(const TestPoolOpts& opts) {
        std::cout << "Testing with parameters: " << opts.to_string() << std::endl;

        ThreadPool<Value> pool{opts.n_workers};

        std::cout << "Testing exception resilency ... " << std::endl;
        {
            Barrier min_threads{opts.n_workers};
            std::atomic_size_t count{0};

            std::vector<std::future<Value>> futures;
            for (size_t i = 0; i < opts.n_workers; ++i) {
                futures.push_back(pool.Submit([i, &min_threads, &count, &opts]() -> Value {
                    min_threads.wait();
                    count_guard guard{count, opts.n_workers};
                    throw ValueException(Value(i, std::to_string(i)));
                }));
            }
            check_exceptions(futures);
            test_assert(count == 0, "an invariant broken");
        }
        std::cout << "Done" << std::endl;

        std::cout << "Testing minimal and maximal concurrency ... " << std::endl;
        {
            Barrier min_threads{std::min(opts.n_workers, opts.n_items)};
            std::atomic_size_t count{0};

            std::vector<std::future<Value>> futures;
            for (size_t i = 0; i < opts.n_items; ++i) {
                futures.push_back(pool.Submit([i, &min_threads, &count, &opts]() {
                    if (i < opts.n_workers) {
                        min_threads.wait();
                    }
                    count_guard guard{count, opts.n_workers};
                    return Value(i, std::to_string(i));
                }));
            }
            check_results(futures);
            test_assert(count == 0, "an invariant broken");
        }
        std::cout << "Done" << std::endl;

        std::cout << "Testing shutdown ... " << std::endl;
        {
            Barrier min_threads{std::min(opts.n_workers, opts.n_items)};
            std::atomic_size_t count{0};

            std::vector<std::future<Value>> futures;
            for (size_t i = 0; i < opts.n_items; ++i) {
                futures.push_back(pool.Submit([i, &min_threads, &count, &opts]() {
                    if (i < opts.n_workers) {
                        min_threads.wait();
                    }
                    count_guard guard{count, opts.n_workers};
                    return Value(i, std::to_string(i));
                }));
            }
            pool.Shutdown();
            check_results(futures);

            bool thrown = false;
            try {
                pool.Submit([]{return Value();});
            } catch (const std::exception&) {
                thrown = true;
            }
            test_assert(thrown, "expected exception");
            test_assert(count == 0, "an invariant broken");
        }
        std::cout << "Done" << std::endl;

        std::cout << "OK" << std::endl;
    }

    void run_pool_tests(int argc, char* argv[]) {
        TestPoolOpts opts;
        read_opts(argc, argv, opts.n_workers, opts.n_items);
        do_test_pool(opts);
    }
}

int main(int argc, char* argv[]) {
//    PoolTests::simulate_ya_contest();
    PoolTests::run_pool_tests(argc, argv);
}
