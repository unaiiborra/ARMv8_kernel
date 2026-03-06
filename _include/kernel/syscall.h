#pragma once

#include <arm/exceptions/exceptions.h>
#include <lib/stdint.h>

typedef enum {
    SYSC_PRINT = 0,

    SYSC_COUNT,
} syscall;


typedef int64 (*syscall_handler)(const uint64 args[6]);


void sysc64_dispatch(arm_exception_ctx* ectx);
