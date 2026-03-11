#include "thread.h"

#include <kernel/hardware.h>
#include <kernel/scheduler.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/lib/smp.h"
#include "kernel/mm.h"
#include "kernel/panic.h"


typedef struct {
    _Alignas(CACHE_LINE) thread* th;
} local_th;


static local_th local_threads[NUM_CORES];

static inline uint64_t get_sp_el0(void)
{
    uint64_t th;
    asm volatile("mrs %0, sp_el0" : "=r"(th) : : "memory");
    return th;
}

void save_current_thread()
{
    thread* cur = get_current_thread();
    DEBUG_ASSERT(cur && ((uintptr_t)cur & KERNEL_BASE) == KERNEL_BASE);

    local_threads[get_cpuid()].th = cur;
}


thread* restore_current_thread(uint64_t* old_sp0)
{
    if (old_sp0)
        *old_sp0 = get_sp_el0();

    thread* cur = local_threads[get_cpuid()].th;
    DEBUG_ASSERT(cur && ((uintptr_t)cur & KERNEL_BASE) == KERNEL_BASE);

    set_current_thread(cur);
    return cur;
}
