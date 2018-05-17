#pragma once

#include "test_utils.h"

#include <pthread.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

namespace SolutionTests {

////////////////////////////////////////
// ya.contest single-core env simulation
////////////////////////////////////////

void simulate_ya_contest() {
    cpu_set_t cpuset;
    pthread_t thread;

    thread = pthread_self();

    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);

    if (0 != pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset)) {
        cthrow("could not pthread_setaffinity_np");
    }

    if (0 != pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset)) {
        cthrow("could not pthread_getaffinity_np");
    }

    for (int j = 1; j < CPU_SETSIZE; j++) {
        if (CPU_ISSET(j, &cpuset)) {
            cthrow("bad thread affinity");
        }
    }
}

}
