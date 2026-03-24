#include <kernel/io/stdio.h>
#include <kernel/scheduler.h>
#include <stddef.h>

#include "../sysc_handlers.h"
#include "kernel/lib/kvec.h"
#include "kernel/panic.h"
#include "lib/lock.h"
#include "lib/stdmacros.h"


int64_t syscall64_exit(
    sysarg_t exit_code,
    sysarg_t a1,
    sysarg_t a2,
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5)
{
    (void)exit_code, (void)a1, (void)a2, (void)a3, (void)a4, (void)a5;

    thread* th = get_current_thread();
    task*   ut = th->owner;

    dbg_printf("[utask: %s] exit code %d", ut->name, exit_code);


    spinlocked(&ut->lock)
    {
        kvec_T(thread*) ths = ut->threads;
        size_t n            = kvec_len(ths);

        for (size_t i = 0; i < n; i++) {
            thread* t;
            dbg_var(bool) res = kvec_get_copy(&ths, i, &t);
            DEBUG_ASSERT(res);

            unschedule_thread(t);
        }
    }


    return 0;
}