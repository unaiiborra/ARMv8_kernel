#pragma once

#include <kernel/process/thread.h>
#include <lib/mem.h>

#include "lib/stdint.h"

extern void _thread_switch_el0(
    v_uintptr pc,
    uint64 fpcr,
    uint64 fpsr,
    uint64 sp,
    uint64 gpr[31],
    uint64 vec[32][2]);


/// returns the last being executed thread of the fn caller core. If no thread
/// is being executed or the thread has been destroyed returns NULL
thread* thread_get_local();

extern void _thread_save_state(void);