#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>


typedef enum {
    // Exit process
    SYSC_EXIT,

    // Print buffer
    SYSC_PRINT,

    // Thread spawn
    SYSC_SPAWN,

    // Thread kill
    SYSC_KILL,

    // Pass control to the kernel and let him decide what to do
    SYSC_YIELD,


    // Not an actual syscall, used for counting the implemented syscall number
    SYSC_COUNT,
} syscall_e;



typedef uint64_t sysarg_t;
extern uint64_t  syscall(
    sysarg_t  arg0,
    sysarg_t  arg1,
    sysarg_t  arg2,
    sysarg_t  arg3,
    sysarg_t  arg4,
    sysarg_t  arg5,
    syscall_e sysc);




/// exit syscall
__attribute__((always_inline)) noreturn static inline void
syscall_exit(uint64_t exit_code)
{
    syscall(exit_code, 0, 0, 0, 0, 0, SYSC_EXIT);

    __builtin_unreachable();
}




/// print syscall
typedef enum {
    SYSC_PRINT_OK          = 0,
    SYSC_PRINT_INVALID_BUF = -1,
} sysc_print_res;


__attribute__((always_inline)) static inline sysc_print_res
syscall_print(const void* buf, size_t size)
{
    return syscall((uint64_t)buf, size, 0, 0, 0, 0, SYSC_PRINT);
}




/// spawn syscall
typedef enum {
    SYSC_SPAWN_RES_OK = 0,
    // requested fn address is not mapped
    SYSC_SPAWN_RES_UNMAPPED = -1,
    // requested fn address region is marked as not executable
    SYSC_SPAWN_RES_NOEXEC = -2,
} sysc_spawn_res;


typedef void (*spawn_fn_t)(uint64_t thid, uint64_t arg);


__attribute__((always_inline)) static inline sysc_spawn_res
syscall_spawn(spawn_fn_t fn, uint64_t arg)
{
    return syscall((uint64_t)fn, 4096 * 4, arg, 0, 0, 0, SYSC_SPAWN);
}




/// kill syscall
typedef enum {
    SYSC_KILL_OK        = 0,
    SYSC_KILL_NOT_FOUND = -1,
} sysc_kill_res;

__attribute__((always_inline)) static inline sysc_kill_res
syscall_kill(uint64_t thid)
{
    return syscall(thid, 0, 0, 0, 0, 0, SYSC_KILL);
}




/// yield syscall
__attribute__((always_inline)) static inline void syscall_yield()
{
    syscall(0, 0, 0, 0, 0, 0, SYSC_YIELD);
}
