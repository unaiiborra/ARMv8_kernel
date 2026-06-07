#pragma once

#include <kernel/io/stdio.h>
#include <kernel/scheduler.h>
#include <stdint.h>

#define SYSC64_SYSCNUM_REG 8
#define SYSC64_RETURN_REG  0
#define SYSC64_RES_UNKNOWN -1000

typedef const uint64_t sysarg_t;
#define unused_sysarg_t __attribute__((unused)) sysarg_t

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

typedef enum : uint64_t {
    // Exit process
    SYSC_EXIT,

    // OPEN file
    SYSC_OPEN,

    // CLOSE file de
    SYSC_CLOSE,

    // READ file descriptor
    SYSC_READ,

    // WRITE file descriptor
    SYSC_WRITE,

    // Thread spawn
    SYSC_SPAWN,

    // Thread kill
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

typedef int64_t (*syscall_handler)(
    sysarg_t a0,
    sysarg_t a1,
    sysarg_t a2,
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5);

int64_t syscall64_exit(
    sysarg_t        exit_code,
    unused_sysarg_t a1,
    unused_sysarg_t a2,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5);

int64_t syscall64_open(
    sysarg_t        path,
    sysarg_t        flags,
    sysarg_t        mode,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5);

int64_t syscall64_close(
    sysarg_t        fd,
    unused_sysarg_t a1,
    unused_sysarg_t a2,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5);

int64_t syscall64_read(
    sysarg_t        fd,
    sysarg_t        buf,
    sysarg_t        count,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5);

int64_t syscall64_write(
    sysarg_t        fd,
    sysarg_t        buf,
    sysarg_t        count,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5);

int64_t syscall64_spawn(
    sysarg_t        fn,
    sysarg_t        arg1, // passed to the spawned thread as x1
    unused_sysarg_t a1,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5);


int64_t syscall64_kill(
    sysarg_t        thid,
    unused_sysarg_t a1,
    unused_sysarg_t a2,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5);

int64_t syscall64_yield(
    unused_sysarg_t a0,
    unused_sysarg_t a1,
    unused_sysarg_t a2,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5);

int64_t syscall64_mmap(
    sysarg_t        addr,
    sysarg_t        lenght,
    sysarg_t        prot,
    sysarg_t        flags,
    unused_sysarg_t fd,    // TODO: implement fs
    unused_sysarg_t offset // TODO: implement fs
);

int64_t syscall64_unmap(
    sysarg_t        addr,
    sysarg_t        lenght,
    unused_sysarg_t a2,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5);
