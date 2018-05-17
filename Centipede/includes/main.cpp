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
        size_t num_foots = 0;
        size_t num_steps = 0;

        TestOpts(int argc, char* argv[]) {
            read_opts(argc, argv, num_foots, num_steps);
        }

        std::string to_string() const {
            std::ostringstream out;
            dump_field(num_foots) << ", ";
            dump_field(num_steps);
            return out.str();
        }
    };

    void do_test(const TestOpts& opts) {
        std::cout << "Starting " << opts.num_foots << " threads ... " << std::endl;

        Barrier barrier_start{opts.num_foots};
        Barrier barrier_end{opts.num_foots};

        Robot centipede{opts.num_foots};

        std::vector<std::thread> foot_threads;

        for (size_t foot = 0; foot < opts.num_foots; ++foot) {
            foot_threads.emplace_back(
                [&, foot, opts]() {
                    barrier_start.wait();

                    for (size_t step = 0; step < opts.num_steps; ++step) {
                        centipede.Step(foot);
                    }

                    barrier_end.wait();
                }
            );
        }

        for (auto& t : foot_threads) {
            t.join();
        }

        std::cout << "Joined " << opts.num_foots << " threads ... " << std::endl;
    }

    void run_tests(int argc, char* argv[]) {
        do_test(TestOpts{argc, argv});
    }
}

int main(int argc, char* argv[]) {
    SolutionTests::run_tests(argc, argv);
}
