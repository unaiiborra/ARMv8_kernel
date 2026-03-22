#pragma once

#include <arm/exceptions/ctx.h>
#include <stddef.h>


typedef enum {
    SYSC_EXIT,
    SYSC_PRINT,

    SYSC_COUNT,
} syscall_e;


void sysc64_dispatch(arm_ectx* ectx);