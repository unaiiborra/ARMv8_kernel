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

    // Map a memory region
    SYSC_MMAP,

    // Unmap a memory region
    SYSC_MUNMAP,


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
noreturn static inline void syscall_exit(uint64_t exit_code)
{
    syscall(exit_code, 0, 0, 0, 0, 0, SYSC_EXIT);

    __builtin_unreachable();
}




/// print syscall
typedef enum {
    SYSC_PRINT_OK          = 0,
    SYSC_PRINT_INVALID_BUF = -1,
} sysc_print_res_e;


static inline sysc_print_res_e syscall_print(const void* buf, size_t size)
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
} sysc_spawn_res_e;


typedef void (*spawn_fn_t)(uint64_t thid, uint64_t arg);


static inline sysc_spawn_res_e syscall_spawn(spawn_fn_t fn, uint64_t arg)
{
    return syscall((uint64_t)fn, 4096 * 4, arg, 0, 0, 0, SYSC_SPAWN);
}




/// kill syscall
typedef enum {
    SYSC_KILL_OK        = 0,
    SYSC_KILL_NOT_FOUND = -1,
} sysc_kill_res_e;

static inline sysc_kill_res_e syscall_kill(uint64_t thid)
{
    return syscall(thid, 0, 0, 0, 0, 0, SYSC_KILL);
}




/// yield syscall
static inline void syscall_yield()
{
    syscall(0, 0, 0, 0, 0, 0, SYSC_YIELD);
}




/// mmap syscall
typedef enum {
    PROT_READ  = 1 << 1,
    PROT_WRITE = 1 << 2,
    PROT_EXEC  = 1 << 3,
} sysc_mmap_prot_e;

typedef enum {
    MAP_ANONYMOUS = 1 << 0, // map not supported by a file
    MAP_FIXED     = 1 << 1, // the provided address is not a suggestion, fail if
                            // overlap
} sysc_mmap_flags_e;

typedef enum {
    // SYSC_MMAP_OK >= 0, (address)
    SYSC_MMAP_ERR               = -1,
    SYSC_MMAP_CFG_NOT_SUPPORTED = -2,
} sysc_mmap_res_e;


static inline int64_t syscall_mmap(
    uintptr_t addr,
    size_t    lenght,
    uint32_t  prot,
    uint32_t  flags,
    uint64_t  fd,
    size_t    offset)
{
    return syscall(addr, lenght, prot, flags, fd, offset, SYSC_MMAP);
}




/// munmap syscall
typedef enum {
    SYSC_MUNMAP_OK                  = 0,
    SYSC_MUNMAP_ERR                 = -1,
    SYSC_MUNMAP_ALIGN_NOT_SUPPORTED = -2,
} sysc_munmap_res_e;

static inline sysc_munmap_res_e syscall_unmap(sysarg_t addr, sysarg_t lenght)
{
    return syscall(addr, lenght, 0, 0, 0, 0, SYSC_MUNMAP);
}