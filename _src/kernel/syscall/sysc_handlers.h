#pragma once

#include <stdint.h>


#define SYSC64_SYSCNUM_REG 8
#define SYSC64_RETURN_REG 0


#define SYSC64_RES_UNKNOWN 0x38

typedef const uint64_t sysarg_t;
typedef int64_t (*syscall_handler)(
    sysarg_t a0,
    sysarg_t a1,
    sysarg_t a2,
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5);

int64_t syscall64_print(
    sysarg_t buf_pt,
    sysarg_t buf_sz,
    sysarg_t a2,
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5);

int64_t syscall64_exit(
    sysarg_t exit_code,
    sysarg_t a1,
    sysarg_t a2,
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5);