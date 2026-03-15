#pragma once

#include <arm/exceptions/exceptions.h>
#include <stddef.h>


typedef enum {
    SYSC_PRINT = 0,

    SYSC_COUNT,
} syscall;


void sysc64_dispatch(arm_exception_ctx* ectx);