#include "solution.h"
#include "test_utils.h"

#include <string>
#include <sstream>
#include <vector>
#include <thread>

namespace SolutionTests {
    const int ITERATIONS = 10000;

    void do_test() {
        std::cout << "Starting 2 threads ... " << std::endl;

        Barrier barrier_start(2);

        Robot robot;
        std::thread left([&barrier_start, &robot]() {
            barrier_start.wait();
            for (int i = 0; i < ITERATIONS; ++i)
                robot.StepLeft();
        });
        std::thread right([&barrier_start, &robot]() {
            barrier_start.wait();
            for (int i = 0; i < ITERATIONS; ++i)
                robot.StepRight();
        });

        left.join();
        right.join();

        std::cout << "Joined 2 threads ... " << std::endl;
    }

    void run_tests() {
        do_test();
    }
}

int main() {
    SolutionTests::run_tests();
}
