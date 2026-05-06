#include <arm/exceptions/ctx.h>
#include <arm/exceptions/exceptions.h>
#include <arm/mmu.h>
#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/io/stdio.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/mm/mmu.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <kernel/smp.h>
#include <kernel/task.h>
#include <kernel/time.h>
#include <lib/branch.h>
#include <lib/lock.h>
#include <lib/stdattribute.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "task.h"
#include "thread.h"


#ifndef DEFAULT_PREEMPTIVE_MICROSEC
#    define DEFAULT_PREEMPTIVE_MICROSEC 3500 /* 3.5 ms */
#endif


static thread* runqueue_schedule();

extern void _scheduler_loop_cpu_enter(arm_ctx* el0_ctx, arm_ctx* el1_ctx);
extern void _scheduler_loop_cpu_exit(arm_ctx* el1_ctx);


typedef struct thread_node {
    struct thread_node *prev, *next;
    thread              th;
} thread_node;


typedef struct {
    _Alignas(CACHE_LINE) corelock_t lock;
    thread_node* list;
    thread*      current_thread;

    atomic_ulong  preemptive_duration_microsec; // not core-local
    timer_event_t preemptive_event;             // core-local

    // for example when a thread calls the syscall yield or when the preemptive
    // timer arrives, this value becomes true. It marks if a schedule call is
    // required to change the current thread
    bool schedule_required; // completely core-local
} runqueue_t;


static arm_ctx pre_sched_mode_ctx[NUM_CPUS];

static runqueue_t runqueue[NUM_CPUS];

static atomic_ulong thread_uid_counter;



static inline thread_node* node_from_thread(thread* th)
{
    return (thread_node*)((char*)th - offsetof(thread_node, th));
}


static inline void set_current_thread(thread* th)
{
    DEBUG_ASSERT(((uintptr_t)th & KERNEL_BASE) == KERNEL_BASE || th == NULL);

    runqueue[get_cpuid()].current_thread = th;
    sysreg_write(sp_el0, th);

    [[maybe_unused]] bool res = mmu_core_set_mapping(
        mm_mmu_core_handler_get_self(),
        th ? &th->owner->mapping : MM_MMU_UNMAPPED_LO);
    DEBUG_ASSERT(res);
}


static void event_preemptive_scheduling([[maybe_unused]] void* ctx)
{
    cpuid_t cpuid = get_cpuid();

    irqlocked()
    {
        runqueue[cpuid].schedule_required      = true;
        runqueue[cpuid].preemptive_event.clock = NULL;
    }
}


void scheduler_init()
{
    sysreg_write(sp_el0, 0);

    atomic_init(&thread_uid_counter, 1);

    for (size_t i = 0; i < NUM_CORES; i++) {
        runqueue[i].list              = NULL;
        runqueue[i].current_thread    = NULL;
        runqueue[i].lock              = CORELOCK_INIT;
        runqueue[i].schedule_required = false;
        atomic_init(
            &runqueue[i].preemptive_duration_microsec,
            DEFAULT_PREEMPTIVE_MICROSEC);

        pre_sched_mode_ctx[i] = (arm_ctx) {0};
    }

    task_ctl_init();
}

#ifdef DEBUG /// fn only valid for debug purposes, use get_current_thread()
             /// instead to avoid memory access (uses sp_el0) and get faster
             /// accesses
thread* __get_current_thread_from_runqueue()
{
    return runqueue[get_cpuid()].current_thread;
}
#endif


void schedule(cpuid_t cpuid)
{
    irqlocked()
    {
        if (runqueue[cpuid].preemptive_event.clock != NULL) {
            timer_cancel_event(runqueue[cpuid].preemptive_event);

            runqueue[cpuid].preemptive_event.clock = NULL;
        }

        runqueue[cpuid].schedule_required = true;
    }
}


uint64_t scheduler_get_preemptive_duration(cpuid_t cpuid)
{
    ASSERT(cpuid < NUM_CPUS);

    irqlocked()
    {
        return atomic_load(&runqueue[cpuid].preemptive_duration_microsec);
    }

    unreachable();
}


void scheduler_set_preemptive_duration(cpuid_t cpuid, uint64_t microseconds)
{
    ASSERT(cpuid < NUM_CPUS);

    irqlocked()
    {
        atomic_store(
            &runqueue[cpuid].preemptive_duration_microsec,
            microseconds);
    }
}


void scheduler_loop_cpu_enter()
{
    cpuid_t cpuid = get_cpuid();

    thread_node* list = runqueue[cpuid].list;

    if (!list)
        return;


    // check that we have at least 1 READY thread
    thread_node *cur, *start;
    cur   = list;
    start = list;

    thread_state expected = THREAD_READY;

    while (!atomic_compare_exchange_strong(
        &cur->th.state,
        &expected,
        THREAD_RUNNING)) {
        cur = cur->next;

        if (cur == start)
            return;

        expected = THREAD_READY;
    }

    thread* th = &cur->th;

    set_current_thread(th);

    arm_exceptions_disable_all();

    runqueue[cpuid].preemptive_event = timer_create_event_delta(
        HRTIMER,
        event_preemptive_scheduling,
        NULL,
        atomic_load(&runqueue[cpuid].preemptive_duration_microsec) * 1000);

    _scheduler_loop_cpu_enter(&th->ctx, &pre_sched_mode_ctx[cpuid]);
}


void scheduler_loop_cpu_exit()
{
    _scheduler_loop_cpu_exit(&pre_sched_mode_ctx[get_cpuid()]);
}


void scheduler_ectx_store(arm_ctx* ectx)
{
    cpuid_t cpuid = get_cpuid();

    core_lock(&runqueue[cpuid].lock);

    // restore into sp_el0 the active thread
    thread* curr = runqueue[get_cpuid()].current_thread;
    DEBUG_ASSERT(curr == NULL || is_kva_ptr(curr));

    if (curr) {
        curr->ctx = *ectx;
    }

    sysreg_write(sp_el0, curr);

    arm_exceptions_enable_all();
}


void scheduler_ectx_load(arm_ctx* ectx)
{
    cpuid_t cpuid = get_cpuid();

    arm_exceptions_disable_all();

    thread* curr = runqueue[cpuid].schedule_required
                       // schedule_required == true: schedule a new thread
                       ? runqueue_schedule()

                       // schedule_required == false: continue with the current
                       // thread
                       : get_current_thread();


    runqueue[cpuid].current_thread = curr;

    if (unlikely(!curr))
        return scheduler_loop_cpu_exit();

    *ectx = curr->ctx;

    core_unlock(&runqueue[cpuid].lock);
}


/// creates a new thread and adds it to the scheduler
thread* schedule_thread(task* owner, uintptr_t entry, bool start_ready)
{
    cpuid_t cpuid = get_cpuid();

    thread_node* node = kmalloc(sizeof(thread_node));
    node->th          = (thread) {
        .th_uid = atomic_fetch_add(&thread_uid_counter, 1),
        .owner  = owner,
        .ctx =
            {.fpcr   = 0,
             .fpsr   = 0,
             .elr    = entry,
             .spsr   = 0, // TODO: allow kernel threads
             .sp_elx = 0x0,
             .x      = {},
             .v      = {}},
        .last_access_time_us = 0,
        .th_flags            = 0,
        .sched_cpu           = cpuid,
    };

    atomic_init(&node->th.state, THREAD_NEW);

    task_add_thread_ref(node->th.owner, &node->th); // sets the local_th_uid
    thread_assign_stack(&node->th);

    corelocked(&runqueue[cpuid].lock)
    {
        thread_node** first = &runqueue[cpuid].list;

        if (unlikely(*first == NULL)) {
            *first     = node;
            node->prev = node;
            node->next = node;
        }
        else {
            thread_node* last = (*first)->prev;

            node->next = *first;
            node->prev = last;

            last->next     = node;
            (*first)->prev = node;
        }
    }


    if (unlikely(start_ready)) {
        thread_state expected = THREAD_NEW;

        bool was_expected = atomic_compare_exchange_strong(
            &node->th.state,
            &expected,
            THREAD_READY);

        // the only reason the thread would not be marked as NEW would be if it
        // was killed, for example by an exit syscall
        ASSERT(was_expected || expected == THREAD_DEAD);
    }

    return &node->th;
}



// removes the thread from the scheduler and from the owner, the lock must be
// taken. Does not free the node
static void unqueue_thread(thread_node* node, cpuid_t runqueue_id)
{
    if (runqueue[runqueue_id].current_thread == &node->th) {
        set_current_thread(NULL);
    }

    if (node->next == node) {
        runqueue[runqueue_id].list = NULL;
    }
    else {
        node->prev->next = node->next;
        node->next->prev = node->prev;

        if (runqueue[runqueue_id].list == node)
            runqueue[runqueue_id].list = node->next;
    }

    task_delete_thread_ref(node->th.owner, &node->th);
}




// defer the deleting of threads
static void free_threads(kvec(thread*) * to_free)
{
    size_t n = kvec_len(to_free);

    for (size_t i = 0; i < n; i++) {
        thread_node* fnode;
        dbgT(bool) res = kvec_get_copy(to_free, i, &fnode);
        DEBUG_ASSERT(res && atomic_load(&fnode->th.state) == THREAD_DEAD);

        dbg_printf(
            DEBUG_TRACE,
            "[%s: %s] thread %d freed",
            (fnode->th.ctx.spsr & 0b1100) == 0 ? "utask" : "ktask",
            fnode->th.owner->name,
            fnode->th.th_uid);

        if (fnode) {
            kfree(fnode);
        }
    }

    kvec_delete(to_free);
}


static thread* runqueue_schedule()
{
    cpuid_t      cpuid = get_cpuid();
    thread*      th    = get_current_thread();
    thread_node* node  = node_from_thread(th);

    ASSERT(th);

    deferT(kvec(thread*), free_threads) to_free = kvec_new(thread_node*);

    corelocked(&runqueue[cpuid].lock)
    {
        DEBUG_ASSERT(th && th->sched_cpu == cpuid);

        // at this point the current thread is the thread that was being
        // executed by the core, its state should still be RUNNING, althought it
        // could also be DEAD (killed by other thread) or SLEEPING

        thread_state
            expected = THREAD_RUNNING; // most probable state is RUNNING,
                                       // then SLEEPING, then DEAD

        if (unlikely(!atomic_compare_exchange_strong(
                &th->state,
                &expected,
                THREAD_READY))) {
            switch (expected) {
                case THREAD_SLEEPING:
                    PANIC("TODO: implement THREAD_SLEEPING");
                    break;
                case THREAD_DEAD:
                    // we own the lock and are the runqueue core so its safe to
                    // remove the thread from the runqueue
                    unqueue_thread(node, cpuid);
                    kvec_push(&to_free, &node);
                    break;
                default:
                    PANIC();
            }
        }

        if (runqueue[cpuid].list == NULL)
            return NULL; // No threads remaining in the runqueue


        // select next thread in the queue
        node     = node->next;
        expected = THREAD_READY;

        while (unlikely(!atomic_compare_exchange_strong(
            &node->th.state,
            &expected,
            THREAD_RUNNING))) {
            switch (expect(expected, THREAD_SLEEPING)) {
                case THREAD_NEW:
                    // NEW threads cannot be executed as they are still being
                    // configured, so just avoid it
                    break;

                case THREAD_DEAD:
                    // we own the lock and are the runqueue core so its safe to
                    // remove the thread from the runqueue
                    unqueue_thread(node, cpuid);
                    kvec_push(&to_free, &node);

                    if (runqueue[cpuid].list == NULL)
                        return NULL; // No threads remaining in the runqueue,
                                     // set by unqueue_thread()

                    break;

                case THREAD_RUNNING:
                    PANIC("no thread should be running because we are scheduling");
                    break;

                case THREAD_SLEEPING:
                    PANIC("TODO: implement THREAD_SLEEPING");
                    break;

                default:
                    PANIC();
            }

            node = node->next; // node has not been freed yet, only removed from
                               // the runqueue, next is valid
            expected = THREAD_READY;
        }

        th = &node->th;
        set_current_thread(th);

        runqueue[cpuid].preemptive_event = timer_create_event_delta(
            HRTIMER,
            event_preemptive_scheduling,
            NULL,
            atomic_load(&runqueue[cpuid].preemptive_duration_microsec) * 1000);
    }

    return th;
}
