#include <arm/exceptions/exceptions.h>
#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/lib/smp.h>
#include <kernel/scheduler.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arm/exceptions/ctx.h"
#include "arm/mmu.h"
#include "arm/reg.h"
#include "kernel/lib/kvec.h"
#include "kernel/mm.h"
#include "kernel/mm/mmu.h"
#include "kernel/panic.h"
#include "lib/lock.h"
#include "task.h"
#include "thread.h"


typedef struct {
    gpr_t fp, lr;

    sysreg_t spsr_el1;
    sp_t sp_el1;
} pre_sched_mode_ctx;


extern void
_scheduler_loop_cpu_enter(arm_ectx* el0_ctx, pre_sched_mode_ctx* el1_ctx);

extern void _scheduler_loop_cpu_exit(pre_sched_mode_ctx* el1_ctx);


typedef struct thread_node {
    struct thread_node *prev, *next;
    thread th;
} thread_node;


typedef struct {
    _Alignas(CACHE_LINE) corelock_t lock;
    thread_node* list;
    thread* current_thread;
} scheduler_t;


static pre_sched_mode_ctx local_pre_sched_mode_ctx[NUM_CORES];

static scheduler_t scheduler[NUM_CORES];

static atomic_ulong thread_uid_counter;


static inline thread_node* node_from_thread(thread* th)
{
    return (thread_node*)((char*)th - offsetof(thread_node, th));
}


static inline void set_current_thread(thread* th)
{
    DEBUG_ASSERT(((uintptr_t)th & KERNEL_BASE) == KERNEL_BASE || th == NULL);

    scheduler[get_cpuid()].current_thread = th;
    sysreg_write(sp_el0, th);
}


void scheduler_init()
{
    sysreg_write(sp_el0, 0);

    atomic_init(&thread_uid_counter, 0);

    for (size_t i = 0; i < NUM_CORES; i++) {
        scheduler[i].list = NULL;
        scheduler[i].current_thread = NULL;
        scheduler[i].lock = CORELOCK_INIT;

        local_pre_sched_mode_ctx[i] = (pre_sched_mode_ctx) {0};
    }

    task_ctl_init();
}


void scheduler_loop_cpu_enter()
{
    cpuid_t cpuid = get_cpuid();

    thread_node* list = scheduler[cpuid].list;

    if (!list)
        return;

    thread* th = &list->th;


    set_current_thread(th);

    bool res = mmu_core_set_mapping(
        mm_mmu_core_handler_get_self(),
        &th->task.utask->mapping);
    ASSERT(res);


    _scheduler_loop_cpu_enter(&th->ctx, &local_pre_sched_mode_ctx[cpuid]);
}


void scheduler_loop_cpu_exit()
{
    _scheduler_loop_cpu_exit(&local_pre_sched_mode_ctx[get_cpuid()]);
}

static thread* schedule()
{
    thread_node* node;
    thread* th = get_current_thread();

    if (th == NULL) { // the current thread was killed by other thread
        node = scheduler[get_cpuid()].list;

        if (node == NULL)
            return NULL;
    }
    else
        node = node_from_thread(th);

    set_current_thread(&node->next->th);

    return &node->next->th;
}


void scheduler_ectx_save(arm_ectx* ectx)
{
    cpuid_t cpuid = get_cpuid();

    core_lock(&scheduler[cpuid].lock);

    // restore into sp_el0 the active thread
    thread* cur = scheduler[get_cpuid()].current_thread;
    DEBUG_ASSERT(cur == NULL || is_kva_ptr(cur));

    if (cur) {
        cur->ctx = *ectx;
    }

    sysreg_write(sp_el0, cur);
}


void schedurer_ectx_restore(arm_ectx* ectx)
{
    cpuid_t cpuid = get_cpuid();
    thread* cur = schedule();
    scheduler[cpuid].current_thread = cur;

    if (!cur)
        return scheduler_loop_cpu_exit();

    *ectx = cur->ctx;

    core_unlock(&scheduler[cpuid].lock);
}


/// creates a new thread and adds it to the scheduler
void schedule_thread(utask* owner, uintptr_t entry)
{
    cpuid_t cpuid = get_cpuid();

    thread_node* node = kmalloc(sizeof(thread_node));
    node->th = (thread) {
        .th_uid = atomic_fetch_add(&thread_uid_counter, 1),
        .task = {.utask = owner},
        .ctx =
            {.fpcr = 0,
             .fpsr = 0,
             .elr = entry,
             .spsr = 0, // TODO: allow kernel threads
             .sp_elx = 0x0,
             .x = {},
             .v = {}},
        .last_access_time_us = 0,
        .th_flags = 0,
        .sched_cpu = cpuid,
        .state = THREAD_NEW,
    };

    spinlocked(&owner->lock)
    {
        corelocked(&scheduler[cpuid].lock)
        {
            thread_node** first = &scheduler[cpuid].list;

            if (*first == NULL) {
                *first = node;
                node->prev = node;
                node->next = node;
            }
            else {
                thread_node* last = (*first)->prev;

                node->next = *first;
                node->prev = last;

                last->next = node;
                (*first)->prev = node;
            }

            thread_assign_stack(&node->th);

            thread* in = &node->th;
            kvec_push(&owner->threads, &in);
        }
    }
}


/// deletes a thread and unschedules it
void unschedule_thread(thread* th)
{
    DEBUG_ASSERT(th);

    thread_node* node;
    cpuid_t th_cpuid = th->sched_cpu;

    corelocked(&scheduler[th_cpuid].lock)
    {
        // if the thread is set as the current, delete it
        if (scheduler[th_cpuid].current_thread == th) {
            if (th_cpuid == get_cpuid()) { // also update the sp_el0 fast access
                set_current_thread(NULL);
            }
            else {
                // no need (nor way) to update sp_el0 of the thread core as we
                // got the lock of the other core, so the sp_el0 is not being
                // used as fast access from the thread cpu_id (it is executing
                // the thread)
                scheduler[th_cpuid].current_thread = NULL;
            }
        }

        node = node_from_thread(th);

        if (node->next == node) {
            scheduler[th_cpuid].list = NULL;
        }
        else {
            node->prev->next = node->next;
            node->next->prev = node->prev;

            if (scheduler[th_cpuid].list == node)
                scheduler[th_cpuid].list = node->next;
        }
    }
}