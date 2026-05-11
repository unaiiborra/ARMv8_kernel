#pragma once

#include <stdint.h>

#include "syscall.h"

/* ── Types ──────────────────────────────────────────────────────────────── */

typedef uint64_t thid_t;

/* ── Return codes ───────────────────────────────────────────────────────── */

typedef enum {
    THREAD_CREATE_UNMAPPED = -1,
    THREAD_CREATE_NOEXEC   = -2,
} thread_create_result_e;

typedef enum {
    THREAD_KILL_NOT_FOUND = -1,
} thread_kill_result_e;

/* ── API ────────────────────────────────────────────────────────────────── */

int64_t thread_create(spawn_fn_t entry, uint64_t arg);
int64_t thread_kill(thid_t thid);