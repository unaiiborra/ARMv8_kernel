#include "thread.h"

#include <arm/sysregs/sysregs.h>
#include <kernel/hardware.h>
#include <kernel/lib/smp.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <lib/lock.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/mm/umalloc.h"
#include "lib/mem.h"


typedef struct {
    _Alignas(CACHE_LINE) thread* th;
} local_th;


static const uintptr_t USR_ADDRSPACE_START = 0x0;
static const uintptr_t USR_ADDRSPACE_END = KERNEL_BASE & ~(0xFFFF000000000000);


/// stores the selected thread of each core to know the thread after returning
/// from el0
static local_th local_threads[NUM_CORES];


void thread_ctl_init()
{
    for (size_t i = 0; i < NUM_CORES; i++)
        local_threads[i].th = NULL;
}


void save_current_thread()
{
    local_threads[get_cpuid()].th = (thread*)sysreg_read(sp_el0);
}


thread* restore_current_thread(uint64_t* old_sp0)
{
    if (old_sp0)
        *old_sp0 = sysreg_read(sp_el0);

    thread* cur = local_threads[get_cpuid()].th;
    DEBUG_ASSERT(cur && ((uintptr_t)cur & KERNEL_BASE) == KERNEL_BASE);

    set_current_thread(cur);
    return cur;
}


static uintptr_t find_free_region(utask* ut, size_t size)
{
    uintptr_t prev_end = USR_ADDRSPACE_START;
    uintptr_t best = UINTPTR_MAX;

    usr_region_node* cur = ut->regions;

    while (cur) {
        uintptr_t start = cur->region.any.usr_start;

        uintptr_t gap = start - prev_end;

        if (gap >= size) {
            uintptr_t candidate = align_down(start - size, KPAGE_SIZE);
            best = candidate;
        }

        prev_end = start + cur->region.any.pages * KPAGE_SIZE;
        cur = cur->next;
    }

    uintptr_t gap = USR_ADDRSPACE_END - prev_end;

    if (gap >= size)
        best = align_down(USR_ADDRSPACE_END - size, KPAGE_SIZE);

    return best;
}


void thread_assign_stack(thread* th)
{
    utask* ut = th->task.utask;

    size_t stack_size = ut->stack_pages * KPAGE_SIZE;

    uintptr_t stack_bottom = find_free_region(ut, stack_size);
    uintptr_t stack_top = stack_bottom + stack_size;
    ASSERT(
        stack_bottom != UINTPTR_MAX && stack_top % 16 == 0 &&
        stack_top <= KERNEL_BASE);

    void* kva = umalloc(
        ut,
        stack_bottom,
        ut->stack_pages,
        true,
        true,
        false,
        true // TODO: change it for non static (allow to free it when
             // deleting a thread)
    );

    memzero64(kva, stack_size);

    th->sp = stack_top;
}