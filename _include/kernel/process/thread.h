#pragma once

#include <arm/cpu.h>
#include <kernel/lib/kvec.h>
#include <lib/mem.h>
#include <lib/stdint.h>

typedef uint64 thread_id;

typedef union {
    ARM_cpu_affinity arm_aff;
    uint32 core_id;
} thread_aff;

typedef struct {
    uint64 fpcr;
    uint64 fpsr;
    uint64 sp;
    uint64 gpr[31];                 // x0-x30
    _Alignas(16) uint64 vec[32][2]; // v0-v31
} thread_regs;

typedef struct {
    v_uintptr stack_bottom; // sp end (lowest addr)
    v_uintptr stack_size;
} thread_stack;

typedef struct {
    thread_id th_id;
    thread_aff th_aff;
    struct proc* owner;

    v_uintptr pc;
    thread_stack stack;
    uint64 tpidr_el0;
    thread_regs regs;
} thread_ctx;


typedef enum {
    THREAD_NEW,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_SLEEPING,
    THREAD_DEAD,
} thread_state;


typedef struct {
    uint32 th_flags;
    thread_state th_state;
    uint64 th_last_ns;
    thread_ctx* th_ctx;
} thread;


struct proc;


void pthread_ctrl_init();

thread* thread_new(struct proc* owner);
bool thread_delete(thread* pth);
bool thread_resume(thread* pth);
