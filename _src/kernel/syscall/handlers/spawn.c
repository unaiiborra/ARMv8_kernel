#include <kernel/mm/umalloc.h>
#include <kernel/scheduler.h>
#include <kernel/task.h>

#include "../sysc_handlers.h"
#include "kernel/mm.h"
#include "kernel/panic.h"


typedef enum {
    SYSC_SPAWN_RES_OK = 0,
    // requested fn address is not mapped
    SYSC_SPAWN_RES_UNMAPPED = -1,
    // requested fn address region is marked as not executable
    SYSC_SPAWN_RES_NOEXEC = -2,
} sysc_spawn_res;

int64_t syscall64_spawn(
    sysarg_t fn,
    sysarg_t stack_sz, // TODO: decide if the stack size is kernel managed or
                       // user manually allocated and managed
    sysarg_t arg1,     // passed as x1
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5)
{
    (void)stack_sz, (void)a3, (void)a4, (void)a5;

    task*        owner  = get_current_thread()->owner;
    task_region* region = NULL;

    bool mapped = uregion_is_mapped(owner, fn, PAGE_SIZE, &region);

    DEBUG_ASSERT(region);

    if (!mapped) {
        dbg_sysc_print(SYSC_SPAWN, "SYSC_SPAWN_RES_UNMAPPED");
        return SYSC_SPAWN_RES_UNMAPPED;
    }

    if (umalloc_get_flag(region, UMALLOC_F_EXEC) == false) {
        dbg_sysc_print(SYSC_SPAWN, "SYSC_SPAWN_RES_NOEXEC");
        return SYSC_SPAWN_RES_NOEXEC;
    }


    thread* new_th = schedule_new_thread(owner, fn);

    new_th->ctx.x[0] = new_th->local_th_uid;
    new_th->ctx.x[1] = arg1;

    thread_promote_to_ready(new_th);


    return SYSC_SPAWN_RES_OK;
}