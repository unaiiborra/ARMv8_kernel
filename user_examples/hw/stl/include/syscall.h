#pragma once

#include <stddef.h>

typedef enum {
    SYSC_PRINT_OK = 0,
    SYSC_PRINT_INVALID_BUF = -1,
} sysc_print_results;


sysc_print_results syscall_print(const void* buf, size_t size);


#define SYSCALL_MAP_PAGE_SIZE 4096

void* syscall_map(size_t pages);