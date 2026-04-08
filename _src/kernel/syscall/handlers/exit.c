#include <kernel/io/stdio.h>
#include <kernel/scheduler.h>
#include <stddef.h>

#include "../sysc_handlers.h"
#include "kernel/lib/kvec.h"
#include "kernel/panic.h"
#include "lib/lock.h"
#include "lib/stdattribute.h"



int64_t syscall64_exit(
    sysarg_t exit_code,
    sysarg_t a1,
    sysarg_t a2,
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5)
{
    (void)exit_code, (void)a1, (void)a2, (void)a3, (void)a4, (void)a5;

    thread* cur = get_current_thread();
    task*   ut  = cur->owner;

    dbg_sysc_print(SYSC_EXIT, "exit code ", exit_code);

    spinlocked(&ut->lock)
    {
        size_t n = kvec_len(ut->threads);

        for (size_t i = 0; i < n; i++) {
            thread* th;
            dbgT(bool) res = kvec_get_copy(&ut->threads, i, &th);
            DEBUG_ASSERT(res);

            unschedule_thread(th);
        }
    }

    return 0;
}