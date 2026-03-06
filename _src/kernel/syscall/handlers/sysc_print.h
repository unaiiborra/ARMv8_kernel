#pragma once

#include <kernel/syscall.h>

typedef enum {
    SYSC_PRINT_OK = 0,
    SYSC_PRINT_INVALID_BUF = -2,
} sysc_print_results;

int64 sysc64_print(const uint64 args[6]);
