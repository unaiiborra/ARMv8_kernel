#pragma once

#include <kernel/io/stdio.h>
#include <kernel/scheduler.h>
#include <stdint.h>


#define SYSC64_SYSCNUM_REG 8
#define SYSC64_RETURN_REG  0


#define SYSC64_RES_UNKNOWN -1000

#define dbg_sysc_print(sysc, msg, ...)                 \
    dbg_printf(                                        \
        DEBUG_TRACE,                                   \
        "[utask: %s thread %d (%d)] (" #sysc ") " msg, \
        get_current_thread()->owner->name,             \
        get_current_thread()->local_th_uid,            \
        get_current_thread()->th_uid,                  \
        ##__VA_ARGS__);



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


int64_t syscall64_spawn(
    sysarg_t fn,
    sysarg_t stack_sz,
    sysarg_t arg1, // passed as x1
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5);


int64_t syscall64_yield(
    sysarg_t a0,
    sysarg_t a1,
    sysarg_t a2,
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5);