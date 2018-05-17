#include "solution.h"
#include "test_utils.h"

#include <atomic>
#include <deque>
#include <string>
#include <vector>
#include <sstream>

namespace QueueTests {

#if defined(dump_field)
#   error "already defined"
#else
#   define dump_field(field) out << (#field) << "=" << (field)
#endif

    struct TestQueueOpts {
        static size_t queue_capacity; // looks ugly, but I had to make it static to simplify passing inside TestingContainer
        size_t n_readers = 10;
        size_t n_writers = 10;
        size_t n_items = 100000;

        std::string to_string() const {
            std::ostringstream out;
            dump_field(queue_capacity) << ", ";
            dump_field(n_readers) << ", ";
            dump_field(n_writers) << ", ";
            dump_field(n_items);
            return out.str();
        }
    };

    size_t TestQueueOpts::queue_capacity = 1000;

    template <typename T>
    class TestingContainer {
        template <typename C>
        struct Check {
            C* cont = nullptr;

            Check(Check&& c) { *this = std::move(c); }

            Check(C* c, bool expect_not_empty = false)
                : cont(c)
            {
                test_assert(cont->impl.size() <= TestQueueOpts::queue_capacity, "capacity exceeded");
                test_assert(!expect_not_empty || !cont->impl.empty(), "expected not empty");
                test_assert(0 == cont->inside_critical.fetch_add(1, std::memory_order_relaxed), "mutual exclusion");
            }

            auto& impl() {
                return cont->impl;
            }

            ~Check() {
                test_assert(cont->impl.size() <= TestQueueOpts::queue_capacity, "capacity exceeded");
                test_assert(1 == cont->inside_critical.fetch_sub(1, std::memory_order_relaxed), "mutual exclusion");
            }

            Check& operator= (Check&& c) {
                if (&c != this) {
                    cont = c.cont;
                    c.cont = nullptr;
                }
                return *this;
            }
        };

        using CheckMutable = Check<TestingContainer<T>>;
        using CheckConst = Check<const TestingContainer<T>>;

    private:
        mutable std::atomic<int> inside_critical{0};
        std::deque<T> impl;

    private:
        CheckMutable get() { return CheckMutable(this); }

        CheckConst get() const { return CheckConst(this); }

        CheckMutable get_no_empty() { return CheckMutable(this, true); }

        CheckConst get_no_empty() const { return CheckConst(this, true); }

    public:
        bool empty() const {
            return get().impl().empty();
        }

        size_t size() const {
            return get().impl().size();
        }

        T& front() {
            return get_no_empty().impl().front();
        }

        const T& front() const {
            return get_no_empty().impl().front();
        }

        void pop_front() {
            get_no_empty().impl().pop_front();
        }

        void push_back(const T& t) {
            get().impl().push_back(t);
        }

        void push_back(T&& t) {
            get().impl().push_back(std::move(t));
        }

        ~TestingContainer() {
            test_assert(get().impl().empty(), "expected empty");
        }
    };

    template <class Queue>
    struct testing_context {
        Queue queue;

        Barrier barrier_begin;
        Barrier barrier_shutdown_enter;
        Barrier barrier_shutdown_exit;

        size_t write_count;
        size_t write_mod;

        size_t read_count;
        size_t read_mod;

        testing_context(const TestQueueOpts& opts)
            : queue{TestQueueOpts::queue_capacity}
            , barrier_begin{opts.n_readers + opts.n_writers + 1}
            , barrier_shutdown_enter{opts.n_readers + opts.n_writers + 1}
            , barrier_shutdown_exit{opts.n_readers + opts.n_writers + 1}
            , write_count{opts.n_items / opts.n_writers}
            , write_mod{opts.n_items % opts.n_writers}
            , read_count{opts.n_items / opts.n_readers}
            , read_mod{opts.n_items % opts.n_readers}
        {}
    };

    void do_test_queue(const TestQueueOpts& opts) {
        std::cout << "Testing with parameters: " << opts.to_string() << std::endl;

        struct Value : public std::pair<std::string, size_t> {
            using Parent = std::pair<std::string, size_t>;

            Value() = default;

            Value(Value&&) = default;
            Value& operator=(Value&&) = default;

            Value(const Value&) = delete;
            Value& operator=(const Value&) = delete;

            Value(const std::string& s, size_t n)
                : Parent(s, n)
            {}

            Value(std::string&& s, size_t n)
                : Parent(std::move(s), n)
            {}
        };

        using Queue = BlockingQueue<Value, TestingContainer<Value>>;
        testing_context<Queue> ctx{opts};

        std::vector<std::thread> writers{opts.n_writers};
        std::vector<std::thread> readers{opts.n_readers};

        std::cout << "Preparing writers ... " << std::endl;
        for (size_t thr = 0; thr < opts.n_writers; ++thr) {
            writers[thr] = std::thread([thr, &ctx, &opts]() {
                try {
                    ctx.barrier_begin.wait();

                    for (size_t i = ctx.write_count * thr, cnt = i + ctx.write_count; i < cnt; ++i) {
                      ctx.queue.Put(Value(std::to_string(i), i));
                    }

                    if (thr < ctx.write_mod) {
                        ctx.queue.Put(Value(std::to_string(thr), thr));
                    }

                    ctx.barrier_shutdown_enter.wait();

                    ctx.barrier_shutdown_exit.wait();

                    bool except = false;

                    try {
                        ctx.queue.Put(Value(std::to_string(thr), thr));
                    } catch (const std::exception&) {
                        except = true;
                    }

                    test_assert(except, "expected an exception because put in shutdown queue");
                } catch (const std::exception& e) {
                    cabort("unexpected exception " << e.what() << typeid(e).name());
                } catch (...) {
                    cabort("unexpected unknown exception");
                }
            });
        }

        std::cout << "Preparing readers ... " << std::endl;
        for (size_t thr = 0; thr < opts.n_readers; ++thr) {
            readers[thr] = std::thread([thr, &ctx, &opts]() {
                try {
                    ctx.barrier_begin.wait();

                    for (size_t i = ctx.read_count * thr, cnt = i + ctx.read_count; i < cnt; ++i) {
                        Value s; 
                        test_assert(ctx.queue.Get(s), "queue is shutdown");
                        test_assert(s.first == std::to_string(s.second), "invalid value found in queue");
                    }

                    if (thr < ctx.read_mod) {
                        Value s;
                        test_assert(ctx.queue.Get(s), "queue is shutdown");
                        test_assert(s.first == std::to_string(s.second), "invalid value found in queue");
                    }

                    ctx.barrier_shutdown_enter.wait();

                    ctx.barrier_shutdown_exit.wait();

                    Value s;
                    test_assert(!ctx.queue.Get(s), "queue isn't shutdown");
                } catch (const std::exception& e) {
                    cabort("unexpected exception " << e.what() << " " << typeid(e).name());
                } catch (...) {
                    cabort("unexpected unknown exception");
                }
            });
        }

        std::cout << "Starting test ... " << std::endl;
        ctx.barrier_begin.wait();
        std::cout << "Done" << std::endl;

        std::cout << "Shutting queue ... " << std::endl;
        ctx.barrier_shutdown_enter.wait();
        std::cout << "Done" << std::endl;

        ctx.queue.Shutdown();

        std::cout << "Shutted queue ... " << std::endl;
        ctx.barrier_shutdown_exit.wait();
        std::cout << "Done" << std::endl;

        std::cout << "Joining writers ... " << std::endl;
        for (auto& thr : writers) {
            if (thr.joinable()) {
                thr.join();
            }
        }
        std::cout << "Done" << std::endl;

        std::cout << "Joining readers ... " << std::endl;
        for (auto& thr : readers) {
            if (thr.joinable()) {
                thr.join();
            }
        }
        std::cout << "Done" << std::endl;

        std::cout << "OK" << std::endl;
    }

    void run_queue_tests(int argc, char* argv[]) {
        TestQueueOpts opts;
        read_opts(argc, argv, TestQueueOpts::queue_capacity, opts.n_readers, opts.n_writers, opts.n_items);
        do_test_queue(opts);
    }
}

int main(int argc, char* argv[]) {
//    QueueTests::simulate_ya_contest();
    QueueTests::run_queue_tests(argc, argv);
}
