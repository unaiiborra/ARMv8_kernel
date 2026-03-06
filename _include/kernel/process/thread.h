#pragma once

#include <arm/cpu.h>
#include <arm/exceptions/exceptions.h>
#include <kernel/lib/kvec.h>
#include <kernel/process/process.h>
#include <lib/lock/corelock.h>
#include <lib/mem.h>
#include <lib/stdint.h>

typedef uint64 thread_id;


typedef struct {
    uint64 sp;
    arm_exception_ctx ectx;
} thread_regs;

typedef struct {
    v_uintptr stack_bottom; // sp end (lowest addr)
    v_uintptr stack_size;
} thread_stack;

typedef struct {
    thread_id th_id;
    uint32 th_aff;
    proc* owner;

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
    corelock_t th_lock;
    thread_state th_state;
    uint64 th_last_time_us;
    thread_ctx* th_ctx;
} thread;


struct proc;


void pthread_ctrl_init();


thread* thread_new(struct proc* owner, bool acquire);
bool thread_delete(thread* pth);
bool thread_try_acquire(thread* pth);
bool thread_resume();

/// releases the resumed thread for other cores to be able to take it
void thread_release_current();


/// should be called after an exception entry from el0 that involves
/// thread or process manipulation or will leave the thread free
void thread_userspace_save(arm_exception_ctx* ectx);

/// should be called when trying to return from kernel space to userspace after
/// an exception has occured
void thread_userspace_restore(arm_exception_ctx* ectx);


/// returns the thread being executed by the caller thread. If a thread has been
/// released returns NULL
static inline thread* thread_get_current()
{
    uintptr p_th;
    asm volatile("mrs %0, sp_el0" : "=r"(p_th));
    return (thread*)p_th;
}
