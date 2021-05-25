#ifndef IFS_SCHEDULER_HPP
#define IFS_SCHEDULER_HPP

#include <agios.h>

void agios_initialize();

void agios_shutdown();

void* agios_callback(int64_t request_id);

void* agios_callback_aggregated(int64_t* requests, int32_t total);

void* agios_eventual_callback(int64_t request_id, void* info);

unsigned long long int generate_unique_id();

#endif