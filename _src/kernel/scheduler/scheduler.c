#include <arm/exceptions/exceptions.h>
#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/lib/smp.h>
#include <kernel/scheduler.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
} scheduler_t;


_Alignas(16) static uint64_t local_el1_ctx[NUM_CORES][4];
static scheduler_t scheduler[NUM_CORES];


static inline thread_node* node_from_thread(thread* th)
{
    return (thread_node*)((char*)th - offsetof(thread_node, th));
}


static void scheduler_init()
{
    asm volatile("msr sp_el0, xzr");

    for (size_t i = 0; i < NUM_CORES; i++) {
        scheduler[i].list = NULL;
        local_el1_ctx[i][0] = 0;
        local_el1_ctx[i][1] = 0;
    }
}

void scheduler_loop_cpu_enter()
{
    static bool sched_init = false;

    __attribute((unused)) uint64_t i;

    if (!sched_init) {
        scheduler_init();
        sched_init = true;
    }

    uint64_t cpuid = get_cpuid();

    thread_node* list = scheduler[cpuid].list;

    if (!list)
        return;

    thread* th = &list->th;


    set_current_thread(th);
    save_current_thread();

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
    cur->pc = _ARM_ELR_EL1();
    cur->sp = sp;
    cur->ctx = *ectx;
}


void schedurer_ectx_restore(arm_exception_ctx* ectx)
{
    thread* cur = schedule();

    if (!cur)
        return scheduler_loop_cpu_exit();

    *ectx = cur->ctx;

    asm volatile("msr sp_el0, %0" : : "r"(cur->pc) : "memory");
    asm volatile("msr sp_el0, %0" : : "r"(cur->sp) : "memory");

    save_current_thread();
}
