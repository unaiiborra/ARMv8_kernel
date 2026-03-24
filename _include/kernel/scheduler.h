#pragma once

#include <arm/exceptions/ctx.h>
#include <arm/mmu.h>
#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/lib/kvec.h>
#include <kernel/lib/smp.h>
#include <kernel/mm.h>
#include <kernel/mm/umalloc.h>
#include <kernel/panic.h>
#include <kernel/task.h>
#include <lib/lock.h>
#include <lib/stdbitfield.h>
#include <stddef.h>
#include <stdint.h>


void scheduler_loop_cpu_enter();
void scheduler_loop_cpu_exit();


void scheduler_ectx_save(arm_ectx* ectx);
void schedurer_ectx_restore(arm_ectx* ectx);


/* --- Tasks --- */


task* task_new(const char* name, size_t stack_size);
void  task_delete(task* ut);


/* --- Threads --- */

typedef enum {
    KERNEL_THREAD,
    USER_THREAD,
} thread_type;


typedef enum {
    THREAD_NEW,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_SLEEPING,
    THREAD_DEAD,
} thread_state;


typedef struct thread {
    uint64_t     th_uid;
    task*        owner;
    arm_ectx     ctx;
    uint64_t     last_access_time_us;
    uint32_t     th_flags;
    cpuid_t      sched_cpu;
    thread_state state;
} thread;


void scheduler_init();


/// creates a new thread and adds it to the scheduler
void schedule_thread(task* owner, uintptr_t entry);

/// deletes a thread and unschedules it
void unschedule_thread(thread* th);


/// calling this function without previously calling scheduler_ectx_save() will
/// cause ub, it is only safe to be called within the saved ctx as it gives fast
/// access by using sp_el0
static inline thread* get_current_thread()
{
    uintptr_t th = sysreg_read(sp_el0);

    DEBUG_ASSERT((th & KERNEL_BASE) == KERNEL_BASE || th == 0);

    return (thread*)th;
}
