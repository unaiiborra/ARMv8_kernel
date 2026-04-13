#pragma once

#include <arm/exceptions/ctx.h>
#include <arm/mmu.h>
#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/mm/umalloc.h>
#include <kernel/panic.h>
#include <kernel/smp.h>
#include <kernel/task.h>
#include <lib/lock.h>
#include <lib/stdbitfield.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>


void scheduler_loop_cpu_enter();
void scheduler_loop_cpu_exit();


void scheduler_ectx_store(arm_ctx* ectx);
void scheduler_ectx_load(arm_ctx* ectx);


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
    uint64_t             th_uid;       // global thread id
    uint64_t             local_th_uid; // thread id local to the task
    task*                owner;
    arm_ctx              ctx;
    uint64_t             last_access_time_us;
    uint32_t             th_flags;
    cpuid_t              sched_cpu;
    _Atomic thread_state state;
} thread;


void scheduler_init();


thread* schedule_thread(task* owner, uintptr_t entry, bool start_ready);

/// creates a new thread and adds it to the scheduler. The thread will be marked
/// as new so it will not execute until marked as READY.
/// thread_promote_to_ready() must be called when the thread is configured.
#define schedule_new_thread(owner, entry) schedule_thread(owner, entry, false)

/// creates a new thread and adds it to the scheduler. The thread will be marked
/// as READY so it will be able to be scheduled directly.
#define schedule_ready_thread(owner, entry) schedule_thread(owner, entry, true)



/// deletes a thread and unschedules it
static inline void unschedule_thread(thread* th)
{
#ifdef DEBUG
    DEBUG_ASSERT(th);
    thread_state old = atomic_exchange(&th->state, THREAD_DEAD);
    DEBUG_ASSERT(old != THREAD_DEAD, "unschedule_thread: double uncheduled");
#else
    atomic_store(&th->state, THREAD_DEAD);
#endif
}


static inline void thread_promote_to_ready(thread* th)
{
    thread_state expected = THREAD_NEW;
    bool         was_new =
        atomic_compare_exchange_strong(&th->state, &expected, THREAD_READY);

    ASSERT(
        was_new,
        "new_thread_mark_as_ready: attempted to mark a thread as READY from a "
        "not NEW thread");
}


#ifdef DEBUG
thread* __get_current_thread_from_runqueue();
#endif

/// calling this function without previously calling scheduler_ectx_store()
/// will cause ub, it is only safe to be called within the saved ctx as it
/// gives fast access by using sp_el0
static inline thread* get_current_thread()
{
    uintptr_t th = sysreg_read(sp_el0);

    DEBUG_ASSERT(
        ((th & KERNEL_BASE) == KERNEL_BASE || th == 0) &&
        th == (uintptr_t)__get_current_thread_from_runqueue());

    return (thread*)th;
}
