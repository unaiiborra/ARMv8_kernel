#include <kernel/syscall.h>
#include <stdint.h>

#include "sysc_handlers.h"


static const syscall_handler SYSC64_TABLE[SYSC_COUNT] = {
    [SYSC_PRINT] = syscall64_print,
};


static inline void sysc64_set_result(arm_exception_ctx* ectx, int64_t result)
{
    ectx->x[SYSC64_RETURN_REG] = result;
}


void sysc64_dispatch(arm_exception_ctx* ectx)
{
    const uint64_t args[6] = {
        ectx->x[0],
        ectx->x[1],
        ectx->x[2],
        ectx->x[3],
        ectx->x[4],
        ectx->x[5],
    };

    const syscall sysc = ectx->x[SYSC64_SYSCNUM_REG];

    if (sysc >= SYSC_COUNT)
        return sysc64_set_result(ectx, SYSC64_RES_UNKNOWN);


    int64_t result = SYSC64_TABLE[sysc](args);

    
    sysc64_set_result(ectx, result);
}