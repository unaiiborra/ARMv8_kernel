#pragma once

#include <arm/exceptions/ctx.h>
#include <kernel/scheduler.h>
#include <stddef.h>


typedef enum {
    // Exit process
    SYSC_EXIT,

    // Print buffer
    SYSC_PRINT,

    // Thread spawn
    SYSC_SPAWN,

    // Thread kill   TODO: implement
    SYSC_KILL,

    // Pass control to the kernel and let him decide what to do
    SYSC_YIELD,



    // Not an actual syscall, used for counting the implemented syscall number
    SYSC_COUNT,
} syscall_e;


void sysc64_dispatch();