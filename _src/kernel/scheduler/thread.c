#include "thread.h"

#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/mm.h>
#include <kernel/mm/umalloc.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <kernel/smp.h>
#include <lib/lock.h>
#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>


void thread_assign_stack(thread* th)
{
    task* t = th->owner;


    spinlocked(&t->lock)
    {
        size_t stack_size = t->stack_pages * PAGE_SIZE;

        uintptr_t stack_bottom = tregion_find_free(t, stack_size);
        uintptr_t stack_top    = stack_bottom + stack_size;
        ASSERT(
            stack_bottom != UINTPTR_MAX && stack_top % 16 == 0 &&
            stack_top <= KERNEL_BASE);

        void* kva = umalloc(
            t,
            stack_bottom,
            t->stack_pages,
            true,
            true,
            false,
            true // TODO: change it for non static (allow to free it when
                 // deleting a thread)
        );

        memzero64(kva, stack_size);

        th->ctx.sp_elx = stack_top;
    }
}
