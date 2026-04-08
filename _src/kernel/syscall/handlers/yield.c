#include "../sysc_handlers.h"



int64_t syscall64_yield(
    sysarg_t a0,
    sysarg_t a1,
    sysarg_t a2,
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5)
{
    (void)a0, (void)a1, (void)a2, (void)a3, (void)a4, (void)a5;

    dbg_sysc_print(SYSC_YIELD, "");

    return 0;
}