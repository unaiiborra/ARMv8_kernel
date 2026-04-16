#include <kernel/io/stdio.h>
#include <kernel/scheduler.h>
#include <stddef.h>

#include "../sysc_handlers.h"
#include "kernel/lib/kvec.h"
#include "kernel/panic.h"
#include "lib/lock.h"
#include "lib/stdattribute.h"



int64_t syscall64_exit(
    sysarg_t                  exit_code,
    [[maybe_unused]] sysarg_t a1,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5)
{
    (void)exit_code;

    thread* cur = get_current_thread();
    task*   ut  = cur->owner;

    dbg_sysc_print(SYSC_EXIT, "exit code ", exit_code);

    spinlocked(&ut->lock)
    {
        size_t n = kvec_len(&ut->threads);

        for (size_t i = 0; i < n; i++) {
            thread* th;
            dbgT(bool) res = kvec_get_copy(&ut->threads, i, &th);
            DEBUG_ASSERT(res);

            unschedule_thread(th);
        }
    }

    return 0;
}