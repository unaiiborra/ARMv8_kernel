#include <kernel/io/stdio.h>
#include <kernel/scheduler.h>
#include <kernel/task.h>
#include <stddef.h>

#include "../sysc_handlers.h"




int64_t syscall64_exit(
    sysarg_t                  exit_code,
    [[maybe_unused]] sysarg_t a1,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5)
{
    dbg_sysc_print(SYSC_EXIT, );
    terminate_task(exit_code);

    return 0;
}