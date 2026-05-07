#pragma once

#include <kernel/io/stdio.h>
#include <kernel/scheduler.h>
#include <stdint.h>


#define SYSC64_SYSCNUM_REG 8
#define SYSC64_RETURN_REG  0


#define SYSC64_RES_UNKNOWN -1000

#if DEBUG == 2
#    define dbg_sysc_print(sysc, msg, ...)                            \
        do {                                                          \
            [[maybe_unused]] thread* __cur_th = get_current_thread(); \
            dbg_printf(                                               \
                DEBUG_TRACE,                                          \
                "[utask: %s thread %d] (" #sysc ") " msg,             \
                __cur_th->owner->name,                                \
                __cur_th->th_uid,                                     \
                ##__VA_ARGS__);                                       \
        } while (0)
#else
#    define dbg_sysc_print(sysc, msg, ...)
#endif


typedef enum {
    // Exit process
    SYSC_EXIT,

    // Print buffer
    SYSC_PRINT,

    // Thread spawn
    SYSC_SPAWN,

    // Thread kill   TODO: implement
    SYSC_KILL,

    // Pass control to the kernel and let him decide what to do
    SYSC_YIELD,

    // Map a memory region
    SYSC_MMAP,

    // Unmap a memory region
    SYSC_MUNMAP,

    // Not an actual syscall, used for counting the implemented syscall number
    SYSC_COUNT,
} syscall_e;


typedef const uint64_t sysarg_t;
typedef int64_t (*syscall_handler)(
    sysarg_t a0,
    sysarg_t a1,
    sysarg_t a2,
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5);

int64_t syscall64_print(
    sysarg_t                  buf_pt,
    sysarg_t                  buf_sz,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5);


int64_t syscall64_exit(
    sysarg_t                  exit_code,
    [[maybe_unused]] sysarg_t a1,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5);


int64_t syscall64_spawn(
    sysarg_t                  fn,
    [[maybe_unused]] sysarg_t stack_sz,
    sysarg_t                  arg1, // passed as x1 to the spawned thread
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5);


int64_t syscall64_kill(
    sysarg_t                  thid,
    [[maybe_unused]] sysarg_t a1,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5);


int64_t syscall64_yield(
    [[maybe_unused]] sysarg_t a0,
    [[maybe_unused]] sysarg_t a1,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5);


int64_t syscall64_mmap(
    sysarg_t                  addr,
    sysarg_t                  lenght,
    sysarg_t                  prot,
    sysarg_t                  flags,
    [[maybe_unused]] sysarg_t fd,    // TODO: implement fs
    [[maybe_unused]] sysarg_t offset // TODO: implement fs
);

int64_t syscall64_unmap(
    sysarg_t                  addr,
    sysarg_t                  lenght,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5);