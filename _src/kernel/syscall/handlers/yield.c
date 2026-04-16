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

    return 0;
}