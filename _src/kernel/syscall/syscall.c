#include <arm/exceptions/exceptions.h>
#include <kernel/process/thread.h>
#include <kernel/syscall.h>
#include <lib/stdint.h>

#include "handlers/sysc_print.h"
#include "kernel/panic.h"
#include "lib/stdmacros.h"


#define SYSCALL_ARG0 0
#define SYSCALL_LAST_ARG 5

#define SYSCALL_N_REG 8
#define RESULT_REG 0


static const syscall_handler SYSCALL_TABLE[SYSC_COUNT] = {
    [SYSC_PRINT] = sysc64_print,
};


static inline void
set_sysc_result(thread* th, arm_exception_ctx* ectx, int64 sysc_result)
{
    th->th_ctx->regs.ectx.x[RESULT_REG] = sysc_result;
    ectx->x[RESULT_REG] = sysc_result;

#ifdef DEBUG
    for (size_t i = 0; i < ARRAY_LEN(ectx->x); i++)
        DEBUG_ASSERT(th->th_ctx->regs.ectx.x[i] == ectx->x[i]);
#endif
}


void sysc64_dispatch(arm_exception_ctx* ectx)
{
    thread* th = thread_get_current();

    const uint64* GPR = th->th_ctx->regs.ectx.x; // x0-x30

    const syscall N = GPR[SYSCALL_N_REG];

    if (N < 0 || N >= SYSC_COUNT) {
        set_sysc_result(th, ectx, -1);
        return;
    }

    uint64 args[6];

    for (size_t i = SYSCALL_ARG0; i <= SYSCALL_LAST_ARG; i++)
        args[i] = GPR[i];

    th->th_ctx->regs.ectx.x[RESULT_REG] = SYSCALL_TABLE[N](args);
}
