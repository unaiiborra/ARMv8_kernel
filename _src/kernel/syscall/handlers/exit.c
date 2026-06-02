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
    dbg_sysc_print(SYSC_EXIT, "exit_code=%d", exit_code);
    terminate_task(get_current_thread()->owner, exit_code);

    return 0;
}
