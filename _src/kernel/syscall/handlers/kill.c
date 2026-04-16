
#include <stddef.h>

#include "../sysc_handlers.h"
#include "kernel/lib/kvec.h"
#include "kernel/panic.h"
#include "kernel/scheduler.h"
#include "lib/lock.h"

typedef enum {
    SYSC_KILL_OK        = 0,
    SYSC_KILL_NOT_FOUND = -1,
} sysc_kill_res;

int64_t syscall64_kill(
    sysarg_t                  thid,
    [[maybe_unused]] sysarg_t a1,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5)
{
    thread* th      = get_current_thread();
    bool    deleted = false;

    irq_spinlocked(&th->owner->lock)
    {
        size_t   n       = kvec_len(&th->owner->threads);
        thread** threads = kvec_data(&th->owner->threads);

        for (size_t i = 0; i < n; i++) {
            if (threads[i] && threads[i]->th_uid == thid) {
                thread_state old_state = unschedule_thread(threads[i]);

                deleted = (old_state != THREAD_DEAD);
                DEBUG_ASSERT(old_state != THREAD_NEW);

                if (deleted) {
                    dbg_sysc_print(
                        SYSC_KILL,
                        "SYSC_KILL_OK thread %d",
                        threads[i]->th_uid);

                    return SYSC_KILL_OK;
                }
                else {
                    dbg_sysc_print(
                        SYSC_KILL,
                        "SYSC_KILL_NOT_FOUND (already deleted thread %d (%d))",
                        threads[i]->th_uid);

                    return SYSC_KILL_NOT_FOUND;
                }
            }
        }
    }


    dbg_sysc_print(SYSC_KILL, "SYSC_KILL_NOT_FOUND (non existant thread)");

    return SYSC_KILL_NOT_FOUND;
}
