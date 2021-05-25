#include <daemon/scheduler/agios.hpp>

unsigned long long int generate_unique_id() {
    // Calculates the hash of this request
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long long int id = ts.tv_sec * 1000000000L + ts.tv_nsec;

    return id;
}