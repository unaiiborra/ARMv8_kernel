#include <kernel/scheduler.h>
#include <kernel/smp.h>

#include "../sysc_handlers.h"



int64_t syscall64_yield(
    [[maybe_unused]] sysarg_t a0,
    [[maybe_unused]] sysarg_t a1,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5)
{
    dbg_sysc_print(SYSC_YIELD, "");

    schedule(get_cpuid());

    return 0;
}