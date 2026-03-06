#pragma once

#include <kernel/process/thread.h>
#include <lib/lock/corelock.h>
#include <lib/mem.h>
#include <lib/stdint.h>


extern void _thread_switch_el0(
    v_uintptr pc,
    uint64 fpcr,
    uint64 fpsr,
    uint64 sp,
    uint64 gpr[31],
    uint64 vec[32][2]);


/// for internal use of proc_new, where the thread needs to be locked but might
/// not be needed to be acquired
thread* thread_new_locked_unaquired(proc* owner);
static inline void thread_unlock(thread* th)
{
    core_unlock(&th->th_lock);
}