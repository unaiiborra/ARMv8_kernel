#include <arm/exceptions/ctx.h>
#include <arm/exceptions/exceptions.h>
#include <arm/mmu.h>
#include <arm/reg.h>
#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/mm/mmu.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <kernel/smp.h>
#include <kernel/task.h>
#include <kernel/time.h>
#include <lib/branch.h>
#include <lib/data_structures/kvec.h>
#include <lib/lock.h>
#include <lib/mem.h>
#include <lib/stdattribute.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include "task.h"

#ifndef SCHEDULER_NUM_RUNQUEUES
constexpr size_t NUM_RUNQUEUES = NUM_CPUS;
#else
constexpr size_t NUM_RUNQUEUES = (SCHEDULER_NUM_RUNQUEUES) > NUM_CPUS
                                     ? NUM_CPUS
                                     : (SCHEDULER_NUM_RUNQUEUES);
#endif

#ifndef DEFAULT_PREEMPTIVE_MICROSEC
#    define DEFAULT_PREEMPTIVE_MICROSEC 3500 /* 3.5 ms */
#endif

static thread_t* runqueue_schedule();

extern void _scheduler_loop_cpu_enter(arm_ctx_t* el0_ctx, arm_ctx_t* el1_ctx);
extern noreturn void _scheduler_loop_cpu_exit(arm_ctx_t* el1_ctx);

typedef struct thread_node {
    struct thread_node *prev, *next;
    thread_t            th;
} thread_node_t;

typedef struct {
    _Alignas(CACHE_LINE) cpulock_t lock;
    thread_node_t* list;
    thread_t*      current_thread;
    atomic_ulong   preemptive_duration_microsec;
    timer_event_t  preemptive_event;
    // for example when a thread calls the syscall yield or when the preemptive
    // timer arrives, this value becomes true. It marks if a schedule call is
    // required to change the current thread
    atomic_bool schedule_required;
} runqueue_t;

static runqueue_t runqueue[NUM_RUNQUEUES];

// locks the runqueue_thread_counts and also serves as a lock that avoids
// deadlocks between 2 runqueues when a thread is deleted and a new thread must
// be stolen from another runqueue
static spinlock_t runqueue_thread_counts_lock;
_Alignas(CACHE_LINE) static uint32_t runqueue_thread_counts[NUM_RUNQUEUES];

static atomic_ulong thread_uid_counter;

static arm_ctx_t pre_sched_mode_ctx[NUM_RUNQUEUES];


static inline thread_node_t* node_from_thread(thread_t* th)
{
    return (thread_node_t*)((char*)th - offsetof(thread_node_t, th));
}


static inline void set_current_thread(thread_t* th)
{
    DEBUG_ASSERT(((uintptr_t)th & KERNEL_BASE) == KERNEL_BASE || th == NULL);

    runqueue[get_cpuid()].current_thread = th;
    sysreg_write(sp_el0, th);
}

static inline void set_thread_mapping(thread_t* th)
{
    maybe_unused bool res = mmu_core_set_mapping(
        mm_mmu_core_handler_get_self(),
        th ? &th->owner->mapping : MM_MMU_UNMAPPED_LO);

    DEBUG_ASSERT(res);
}

static void event_preemptive_scheduling(maybe_unused void* ctx)
{
    cpuid_t cpuid = get_cpuid();

    irqlocked()
    {
        atomic_store(&runqueue[cpuid].schedule_required, true);
        runqueue[cpuid].preemptive_event.clock = NULL;
    }
}

void scheduler_init()
{
    sysreg_write(sp_el0, 0);

    atomic_init(&thread_uid_counter, 1);
    runqueue_thread_counts_lock = SPINLOCK_INIT;

    for (size_t i = 0; i < NUM_RUNQUEUES; i++) {
        runqueue[i].list           = NULL;
        runqueue[i].current_thread = NULL;
        runqueue[i].lock           = CPULOCK_INIT;
        atomic_init(&runqueue[i].schedule_required, false);
        atomic_init(
            &runqueue[i].preemptive_duration_microsec,
            DEFAULT_PREEMPTIVE_MICROSEC);

        pre_sched_mode_ctx[i]     = (arm_ctx_t) {0};
        runqueue_thread_counts[i] = 0;
    }

    task_ctl_init();
}

#ifdef DEBUG /// fn only valid for debug purposes, use get_current_thread()
             /// instead to avoid memory access (uses sp_el0) and get faster
             /// accesses
thread_t* __get_current_thread_from_runqueue()
{
    return runqueue[get_cpuid()].current_thread;
}
#endif

void schedule(cpuid_t cpuid)
{
    cpulocked_irqsave(&runqueue[cpuid].lock)
    {
        if (runqueue[cpuid].preemptive_event.clock != NULL) {
            timer_cancel_event(runqueue[cpuid].preemptive_event);

            runqueue[cpuid].preemptive_event.clock = NULL;
        }

        atomic_store(&runqueue[cpuid].schedule_required, true);
    }
}

// must be called with the runqueue lock taken
static bool schedule_is_required(cpuid_t cpuid)
{
    thread_state state = atomic_load(&get_current_thread()->state);

    return state == THREAD_SLEEPING || state == THREAD_DEAD ||
           atomic_load(&runqueue[cpuid].schedule_required);
}

uint64_t scheduler_get_preemptive_duration(cpuid_t cpuid)
{
    ASSERT(cpuid < NUM_RUNQUEUES);

    irqlocked()
    {
        return atomic_load(&runqueue[cpuid].preemptive_duration_microsec);
    }

    unreachable();
}

void scheduler_set_preemptive_duration(cpuid_t cpuid, uint64_t microseconds)
{
    ASSERT(cpuid < NUM_RUNQUEUES);

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

    if (cpuid > NUM_RUNQUEUES) {
        dbg_printf(
            DEBUG_TRACE,
            "[CORE %d] exited runqueue due to explicit NUM_RUNQUEUES=%d\n\r",
            cpuid,
            (uint32_t)NUM_RUNQUEUES);
        return;
    }


    thread_node_t* list = runqueue[cpuid].list;

    if (!list)
        return;

    // check that we have at least 1 READY thread
    thread_node_t *cur, *start;
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

    thread_t* th = &cur->th;

    arm_exceptions_disable_all();

    set_current_thread(th);
    set_thread_mapping(th);

    runqueue[cpuid].preemptive_event = timer_create_event_delta(
        HRTIMER,
        event_preemptive_scheduling,
        NULL,
        atomic_load(&runqueue[cpuid].preemptive_duration_microsec) * 1000);

    memzero(&pre_sched_mode_ctx[cpuid], sizeof(arm_ctx_t));

    _scheduler_loop_cpu_enter(&th->ctx, &pre_sched_mode_ctx[cpuid]);
}

noreturn void scheduler_loop_cpu_exit()
{
    _scheduler_loop_cpu_exit(&pre_sched_mode_ctx[get_cpuid()]);
    PANIC();
}

void scheduler_ectx_store(arm_ctx_t* ectx)
{
    cpuid_t cpuid = get_cpuid();

    cpulock_acquire(&runqueue[cpuid].lock);

    // restore into sp_el0 the active thread
    thread_t* curr = runqueue[get_cpuid()].current_thread;
    DEBUG_ASSERT(curr == NULL || is_kva_ptr(curr));

    if (curr) {
        memcpy(&curr->ctx, ectx, sizeof(arm_ctx_t));
    }

    sysreg_write(sp_el0, curr);

    arm_exceptions_enable_all();
}

void scheduler_ectx_load(arm_ctx_t* ectx)
{
    cpuid_t cpuid = get_cpuid();

    arm_exceptions_disable_all();


    thread_t* curr = schedule_is_required(cpuid)
                         // schedule_required == true:
                         // schedule a new thread
                         ? runqueue_schedule()

                         // schedule_required ==
                         // false: continue with the
                         // current thread
                         : get_current_thread();


    runqueue[cpuid].current_thread = curr;

    if (unlikely(!curr))
        return scheduler_loop_cpu_exit();

    *ectx = curr->ctx;
    set_thread_mapping(curr);

    cpulock_release(&runqueue[cpuid].lock);
}

static void runqueue_insert_node(thread_node_t* node, cpuid_t runqueue_idx)
{
    DEBUG_ASSERT(cpulock_is_locked(&runqueue[runqueue_idx].lock));
    DEBUG_ASSERT(runqueue_idx < NUM_RUNQUEUES);

    node->th.sched_cpu = runqueue_idx;

    thread_node_t** first = &runqueue[runqueue_idx].list;

    if (unlikely(*first == NULL)) {
        *first     = node;
        node->prev = node;
        node->next = node;
    }
    else {
        thread_node_t* last = (*first)->prev;

        node->next = *first;
        node->prev = last;

        last->next     = node;
        (*first)->prev = node;
    }

    DEBUG_ASSERT(runqueue_thread_counts[runqueue_idx] < UINT32_MAX);
    runqueue_thread_counts[runqueue_idx]++;
}

static void runqueue_remove_node(thread_node_t* node, cpuid_t runqueue_idx)
{
    DEBUG_ASSERT(cpulock_is_locked(&runqueue[runqueue_idx].lock));
    DEBUG_ASSERT(runqueue_idx < NUM_RUNQUEUES);
    DEBUG_ASSERT(runqueue_idx == node->th.sched_cpu);

    if (node->next == node) {
        runqueue[runqueue_idx].list = NULL;
    }
    else {
        node->prev->next = node->next;
        node->next->prev = node->prev;

        if (runqueue[runqueue_idx].list == node)
            runqueue[runqueue_idx].list = node->next;
    }

    DEBUG_ASSERT(runqueue_thread_counts[runqueue_idx] != 0);
    runqueue_thread_counts[runqueue_idx]--;
}

/// creates a new thread and adds it to the scheduler
thread_t* schedule_thread(task_t* owner, uintptr_t entry, bool start_ready)
{
    thread_node_t* node = kmalloc(sizeof(thread_node_t));

    spinlocked(&owner->threads_lock) spinlocked(&runqueue_thread_counts_lock)
    {
        // 1. choose best runqueue for the new thread
        cpuid_t  runqueue_idx = 0;
        uint32_t tmin         = UINT32_MAX;
        uint32_t gmin         = UINT32_MAX;

        for (size_t i = 0; i < NUM_RUNQUEUES; i++) {
            // task thread runqueue count
            uint32_t t = owner->threads_per_cpu[i];

            // global runqueue count
            uint32_t g = runqueue_thread_counts[i];

            if (t < tmin || (t == tmin && g < gmin)) {
                tmin         = t;
                gmin         = g;
                runqueue_idx = i;
            }
        }

        // 2. initialize thread
        node->th = (thread_t) {
            .th_uid = atomic_fetch_add(&thread_uid_counter, 1),
            .owner  = owner,
            .ctx =
                {.fpcr   = 0,
                 .fpsr   = 0,
                 .elr    = entry,
                 .spsr   = 0x0, // TODO: allow kernel threads
                 .sp_elx = 0x0,
                 .x      = {[0 ... XREG_COUNT - 1] = 0},
                 .v      = {[0 ... VREG_COUNT - 1] = 0}},
            .last_access_time_us = 0,
            .sched_cpu           = runqueue_idx,
        };

        atomic_init(&node->th.state, THREAD_NEW);

        // 3. Add reference to the owner (already adds +1 to the threads_per_cpu)
        task_add_thread_ref(node->th.owner, &node->th, runqueue_idx);

        // 4. Insert to the runqueue
        cpulocked(&runqueue[runqueue_idx].lock)
        {
            runqueue_insert_node(node, runqueue_idx);
        }
    }

    if (start_ready) {
        thread_state expected = THREAD_NEW;

        maybe_unused bool was_expected = atomic_compare_exchange_strong(
            &node->th.state,
            &expected,
            THREAD_READY);

        // the only reason the thread would not be marked as NEW would be if
        // it was killed, for example by an exit syscall
        DEBUG_ASSERT(was_expected || expected == THREAD_DEAD);
    }

    return &node->th;
}

// removes the thread from the scheduler and from the owner, the runqueue lock
// must be taken. Does not free the node
static void unqueue_thread(thread_node_t* node, cpuid_t runqueue_idx)
{
    DEBUG_ASSERT(cpulock_is_locked(&runqueue[runqueue_idx].lock));

    // 1. node removal
    if (runqueue[runqueue_idx].current_thread == &node->th)
        set_current_thread(NULL);

    // the node is just removed, the caller must free it
    runqueue_remove_node(node, runqueue_idx);

    spinlocked(&node->th.owner->threads_lock)
    {
        task_t*           task    = node->th.owner;
        maybe_unused bool deleted = task_try_delete_thread_ref(task, &node->th);
        DEBUG_ASSERT(deleted);

        // 2. load balancing
        spinlocked_irqsave(&runqueue_thread_counts_lock)
        {
            cpuid_t  victim_cpu = 0;
            uint32_t tmin       = task->threads_per_cpu[runqueue_idx];

            // find most loaded cpu for this task, if some runqueues share
            // values get compare with global
            uint32_t tmax = 0, gmax = 0;

            for (size_t i = 0; i < NUM_RUNQUEUES; i++) {
                // task thread runqueue count
                uint32_t t = task->threads_per_cpu[i];

                // global runqueue count
                uint32_t g = runqueue_thread_counts[i];

                if (t > tmax || (t == tmax && g > gmax)) {
                    tmax       = t;
                    gmax       = g;
                    victim_cpu = i;
                }
            }

            // delta == quota || delta == quota + 1 (fully task balanced)
            if (tmax - tmin <= 1)
                return; // already task balanced

            thread_t* victim = task_get_any_thread_with_sched_cpu(
                task,
                victim_cpu);

            DEBUG_ASSERT(victim && victim->sched_cpu == victim_cpu);

            thread_node_t* victim_node = node_from_thread(victim);

            // remove thread from victim's runqueue
            cpulocked(&runqueue[victim_cpu].lock)
            {
                runqueue_remove_node(victim_node, victim_cpu);
            }

            // insert the stolen node to the self runqueue, locked from the caller
            runqueue_insert_node(victim_node, runqueue_idx);

            task->threads_per_cpu[runqueue_idx]++;
            task->threads_per_cpu[victim_cpu]--;
        }
    }
}

// defer the deleting of threads
static void free_threads(kvec(thread*) * to_free)
{
    size_t n = kvec_len(to_free);

    for (size_t i = 0; i < n; i++) {
        thread_node_t*    fnode;
        maybe_unused bool res = kvec_get_copy(to_free, i, &fnode);
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


static thread_t* runqueue_schedule()
{
    const cpuid_t        cpuid = get_cpuid();
    thread_t* const      curr  = get_current_thread();
    thread_node_t* const node  = node_from_thread(curr);

    ASSERT(curr);

    deferT(kvec(thread*), free_threads) to_free = kvec_new(thread_node_t*);


    cpulocked(&runqueue[cpuid].lock)
    {
        DEBUG_ASSERT(curr && curr->sched_cpu == cpuid);

        // at this point the current thread is the thread that was being
        // executed by the core, its state should still be RUNNING, althought it
        // could also be DEAD (killed by other thread) or SLEEPING


        /* --- handle the current thread's state --- */
        thread_state
            expected = THREAD_RUNNING; // most probable state is RUNNING,
                                       // then SLEEPING, then DEAD

        if (unlikely(!atomic_compare_exchange_strong(
                &curr->state,
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
            goto null_return; // No threads remaining in the runqueue

        /* --- select a new valid thread as scheduled --- */
        thread_node_t* selected_node = node;

        while (true) {
            // select next thread in the queue
            selected_node = selected_node->next;
            expected      = THREAD_READY;

            // check that the thread's owner is not dying
            task_state_e owner_state = atomic_load(
                &selected_node->th.owner->state);

            DEBUG_ASSERT(
                owner_state != TASK_DEAD,
                "no threads of a dead task should be in the runqueue");

            if (unlikely(owner_state == TASK_DYING)) {
                atomic_store(&selected_node->th.state, THREAD_DEAD);
                unqueue_thread(selected_node, cpuid);
                kvec_push(&to_free, &selected_node);

                if (runqueue[cpuid].list == NULL)
                    goto null_return;

                continue;
            }

            bool scheduled_as_ready = atomic_compare_exchange_strong(
                &selected_node->th.state,
                &expected,
                THREAD_RUNNING);

            if (likely(scheduled_as_ready))
                break; // exit the loop

            // The thread was not READY, check the state it was in
            switch (expected) {
                case THREAD_NEW: {
                    // NEW threads cannot be executed as they are still
                    // being configured, so just avoid it
                } break;

                case THREAD_DEAD: {
                    // we own the lock and are the runqueue core so its safe
                    // to remove the thread from the runqueue
                    unqueue_thread(selected_node, cpuid);
                    kvec_push(&to_free, &selected_node);

                    if (runqueue[cpuid].list == NULL)
                        goto null_return; // No threads remaining in the
                                          // runqueue, set by unqueue_thread()
                } break;

                case THREAD_RUNNING: {
                    PANIC(
                        "no thread should be running because we are "
                        "scheduling");
                }

                case THREAD_SLEEPING: {
                    PANIC("TODO: implement THREAD_SLEEPING");
                }

                default: {
                    PANIC();
                }
            }
        }

        runqueue[cpuid].preemptive_event = timer_create_event_delta(
            HRTIMER,
            event_preemptive_scheduling,
            NULL,
            atomic_load(&runqueue[cpuid].preemptive_duration_microsec) * 1000);

        set_current_thread(&selected_node->th);
        return &selected_node->th;
    }

    PANIC("unreachable");

null_return: {
    set_current_thread(NULL);
    return NULL;
}
}
