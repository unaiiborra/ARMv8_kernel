#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

/* ── Mmap prot flags ────────────────────────────────────────────────────── */

#define MMAP_PROT_READ  (1 << 0)
#define MMAP_PROT_WRITE (1 << 1)
#define MMAP_PROT_EXEC  (1 << 2)

/* ── Mmap flags ─────────────────────────────────────────────────────────── */

#define MMAP_FLAG_ANONYMOUS (1 << 0)
#define MMAP_FLAG_FIXED     (1 << 1)

/* ── Return codes ───────────────────────────────────────────────────────── */

typedef enum {
    SYSCALL_PRINT_INVALID_BUF = -1,
} syscall_print_result_e;

typedef enum {
    SYSCALL_SPAWN_UNMAPPED = -1,
    SYSCALL_SPAWN_NOEXEC   = -2,
} syscall_spawn_result_e;

typedef enum {
    SYSCALL_KILL_NOT_FOUND = -1,
} syscall_kill_result_e;

typedef enum {
    SYSCALL_MMAP_ERR               = -1,
    SYSCALL_MMAP_CFG_NOT_SUPPORTED = -2,
} syscall_mmap_result_e;

typedef enum {
    SYSCALL_MUNMAP_ERR                 = -1,
    SYSCALL_MUNMAP_ALIGN_NOT_SUPPORTED = -2,
} syscall_munmap_result_e;

/* ── Types ──────────────────────────────────────────────────────────────── */

typedef void (*spawn_fn_t)(uint64_t thid, uint64_t arg);

/* ── Syscalls ───────────────────────────────────────────────────────────── */

__attribute__((__noreturn__)) void syscall_exit(int32_t code);
int64_t syscall_print(const uint8_t* buf, size_t size);
int64_t syscall_spawn(spawn_fn_t entry, uint64_t arg);
int64_t syscall_kill(uint64_t thid);
void    syscall_yield(void);
int64_t syscall_mmap(
    void*    addr,
    size_t   length,
    uint64_t prot,
    uint64_t flags,
    uint64_t fd,
    size_t   offset);
int64_t syscall_munmap(void* addr, size_t length);
