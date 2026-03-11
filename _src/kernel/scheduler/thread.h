#pragma once

#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>


static inline thread* get_current_thread(void)
{
    uintptr_t th;

    asm volatile("mrs %0, sp_el0" : "=r"(th) : : "memory");

    DEBUG_ASSERT((th & KERNEL_BASE) == KERNEL_BASE || th == 0);

    return (thread*)th;
}


static inline void set_current_thread(thread* th)
{
    asm volatile("msr sp_el0, %0" : : "r"(th) : "memory");
}


void save_current_thread();
thread* restore_current_thread(uint64_t* old_sp0);
