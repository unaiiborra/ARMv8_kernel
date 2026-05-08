#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t thread_id_t;

typedef void (*thread_start_t)(thread_id_t thid, void* arg);

bool thread_create(thread_start_t entry, void* arg);
bool thread_exit(thread_id_t thid);
void thread_yield();
