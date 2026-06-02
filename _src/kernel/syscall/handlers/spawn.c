#include <kernel/mm/uregion.h>
#include <kernel/scheduler.h>
#include <kernel/task.h>
#include <stdint.h>

#include "../sysc_handlers.h"
#include "lib/branch.h"
#include "lib/lock.h"



typedef enum {
    // spawned thid >= 0,

    // requested fn address is not mapped
    SYSC_SPAWN_RES_UNMAPPED = -1,
    // requested fn address region is marked as not executable
    SYSC_SPAWN_RES_NOEXEC = -2,
} sysc_spawn_res_e;


int64_t syscall64_spawn(
    sysarg_t                  fn,
    sysarg_t                  arg1,
    [[maybe_unused]] sysarg_t a1,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5)
{
    task_t*    task   = get_current_thread()->owner;
    uregion_t* region = NULL;

    spinlocked(&task->lock)
    {
        bool mapped = uregion_is_reserved(task, fn, 4, &region);

        if (unlikely(!mapped)) {
            dbg_sysc_print(SYSC_SPAWN, "SYSC_SPAWN_RES_UNMAPPED %p", fn);

            return SYSC_SPAWN_RES_UNMAPPED;
        }


        if (unlikely((uregion_get_flags(region) & UREGION_F_EXEC) == 0)) {
            dbg_sysc_print(SYSC_SPAWN, "SYSC_SPAWN_RES_NOEXEC");

            return SYSC_SPAWN_RES_NOEXEC;
        }
    }

    thread* new_th = schedule_new_thread(task, fn);

    uint64_t thid = new_th->th_uid;

    new_th->ctx.x[0] = thid;
    new_th->ctx.x[1] = arg1;

    thread_promote_to_ready(new_th);


    return thid;
}
