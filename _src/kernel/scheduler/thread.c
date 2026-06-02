#include "thread.h"

#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/mm.h>
#include <kernel/mm/uregion.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <kernel/smp.h>
#include <lib/lock.h>
#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/task.h"


void thread_assign_stack(thread* th)
{
    task_t* t = th->owner;


    spinlocked(&t->lock)
    {
        size_t    stack_size   = t->stack_pages * PAGE_SIZE;
        uintptr_t stack_bottom = uregion_find_free(t, t->stack_pages);
        uintptr_t stack_top    = stack_bottom + stack_size;

        ASSERT(
            stack_bottom != UINTPTR_MAX && stack_top % 16 == 0 &&
            stack_top <= KERNEL_BASE);

        uregion_result_e ureg = uregion_reserve(
            t,
            stack_bottom,
            t->stack_pages,
            true,
            true,
            false);

        ASSERT(ureg == UREGION_OK);

        void* kva = uregion_commit(t, stack_bottom, t->stack_pages);

        ASSERT(kva);

        memzero64(kva, stack_size);

        th->ctx.sp_elx = stack_top;
    }
}
