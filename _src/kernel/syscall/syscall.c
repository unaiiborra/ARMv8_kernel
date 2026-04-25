#include <arm/exceptions/ctx.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <stddef.h>
#include <stdint.h>

#include "sysc_handlers.h"


static const syscall_handler SYSC64_TABLE[SYSC_COUNT] = {
    [SYSC_EXIT]  = syscall64_exit,
    [SYSC_PRINT] = syscall64_print,
    [SYSC_SPAWN] = syscall64_spawn,
    [SYSC_KILL]  = syscall64_kill,
    [SYSC_YIELD] = syscall64_yield,
};


static inline void sysc64_set_result(arm_ctx* ctx, int64_t result)
{
    ctx->x[SYSC64_RETURN_REG] = result;
}


void sysc64_dispatch()
{
    arm_ctx* ctx = &get_current_thread()->ctx;

    size_t sysc = ctx->x[SYSC64_SYSCNUM_REG];

    if (sysc >= SYSC_COUNT)
        return sysc64_set_result(ctx, SYSC64_RES_UNKNOWN);

    int64_t result = SYSC64_TABLE[sysc](
        ctx->x[0],
        ctx->x[1],
        ctx->x[2],
        ctx->x[3],
        ctx->x[4],
        ctx->x[5]);


    sysc64_set_result(ctx, result);
}