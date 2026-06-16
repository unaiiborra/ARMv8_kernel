#include <kernel/scheduler.h>
#include <kernel/smp.h>

#include "../sysc_handlers.h"



int64_t syscall64_yield(
    unused_sysarg_t a0,
    unused_sysarg_t a1,
    unused_sysarg_t a2,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5)
{
    dbg_sysc_print(SYSC_YIELD, "");

    schedule(get_cpuid());

    return 0;
}