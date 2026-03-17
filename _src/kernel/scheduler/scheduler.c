#include <arm/exceptions/exceptions.h>
#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/lib/smp.h>
#include <kernel/scheduler.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arm/mmu.h"
#include "kernel/mm.h"
#include "kernel/mm/mmu.h"
#include "kernel/panic.h"
#include "lib/lock.h"
#include "task.h"
#include "thread.h"


extern void _scheduler_loop_cpu_enter(
    arm_exception_ctx* el0_ctx,
    uint64_t sp_el0,
    uint64_t pc,
    uint64_t el1_ctx[4]);


extern void _scheduler_loop_cpu_exit(uint64_t el1_ctx[4]);


typedef struct thread_node {
    struct thread_node *prev, *next;
    thread th;
} thread_node;


typedef struct {
    _Alignas(CACHE_LINE) thread_node* list;
    spinlock_t lock;
} scheduler_t;


_Alignas(16) static uint64_t local_el1_ctx[NUM_CORES][4];
static scheduler_t scheduler[NUM_CORES];

static atomic_ulong thread_uid_counter;


static inline thread_node* node_from_thread(thread* th)
{
    return (thread_node*)((char*)th - offsetof(thread_node, th));
}


void scheduler_init()
{
    sysreg_write(sp_el0, 0);

    atomic_init(&thread_uid_counter, 0);

    for (size_t i = 0; i < NUM_CORES; i++) {
        scheduler[i].list = NULL;
        spinlock_init(&scheduler[i].lock);

        for (size_t j = 0; j < 4; j++)
            local_el1_ctx[i][j] = 0;
    }

    task_ctl_init();
    thread_ctl_init();
}


void scheduler_loop_cpu_enter()
{
    cpuid_t cpuid = get_cpuid();

    thread_node* list = scheduler[cpuid].list;

    if (!list)
        return;

    thread* th = &list->th;


    set_current_thread(th);
    save_current_thread();

    bool res = mmu_core_set_mapping(
        mm_mmu_core_handler_get_self(),
        &th->task.utask->mapping);
    ASSERT(res);


    _scheduler_loop_cpu_enter(
        &th->ctx,
        th->sp,
        th->pc,
        &local_el1_ctx[cpuid][0]);
}


void scheduler_loop_cpu_exit()
{
    _scheduler_loop_cpu_exit(&local_el1_ctx[get_cpuid()][0]);
}

static thread* schedule()
{
    thread* th = get_current_thread();
    thread_node* node = node_from_thread(th);

    set_current_thread(&node->next->th);
    return &node->next->th;
}


void scheduler_ectx_save(arm_exception_ctx* ectx)
{
    // restore into sp_el0 the active thread
    uint64_t sp;
    thread* cur = restore_current_thread(&sp);
    cur->pc = sysreg_read(elr_el1);
    cur->sp = sp;
    cur->ctx = *ectx;
}


void schedurer_ectx_restore(arm_exception_ctx* ectx)
{
    thread* cur = schedule();

    if (!cur)
        return scheduler_loop_cpu_exit();

    *ectx = cur->ctx;


    sysreg_write(elr_el1, cur->pc);
    sysreg_write(sp_el0, cur->sp);


    save_current_thread();
}


/// creates a new thread and adds it to the scheduler
void schedule_thread(utask* owner, uintptr_t entry)
{
    cpuid_t cpuid = get_cpuid();

    spin_lock(&owner->lock);
    spin_lock(&scheduler[cpuid].lock); // TOOD: dynamic core selection


    thread_node* node = kmalloc(sizeof(thread_node));

    node->th = (thread) {
        .th_uid = atomic_fetch_add(&thread_uid_counter, 1),
        .task = {.utask = owner},
        .sp = 0x0,
        .pc = entry,
        .ctx = {.fpcr = 0, .fpsr = 0, .x = {}, .v = {}},
        .last_access_time_us = 0,
        .th_flags = 0,
    };


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


    spin_unlock(&scheduler[cpuid].lock);
    spin_unlock(&owner->lock);
}


/// deletes a thread and unschedules it
void unschedule_thread(utask* owner, uintptr_t entry);