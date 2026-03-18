#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>


typedef enum {
    SYSC_EXIT,
    SYSC_PRINT,

    SYSC_COUNT,
} syscall_e;


typedef uint64_t sysarg_t;
extern uint64_t syscall(
    sysarg_t arg0,
    sysarg_t arg1,
    sysarg_t arg2,
    sysarg_t arg3,
    sysarg_t arg4,
    sysarg_t arg5,
    syscall_e sysc);


noreturn void syscall_exit(uint64_t exit_code);


typedef enum {
    SYSC_PRINT_OK = 0,
    SYSC_PRINT_INVALID_BUF = -1,
} sysc_print_results;

sysc_print_results syscall_print(const void* buf, size_t size);

