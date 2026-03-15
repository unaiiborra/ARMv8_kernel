#pragma once

#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>


void thread_ctl_init();


/// stores the current set thread for restoring after returning from el0.
void save_current_thread();

/// restores the thread that was being executed in el0 into the sp0 fast access.
/// Returns the restored thread ptr
thread* restore_current_thread(uint64_t* old_sp0);


void thread_assign_stack(thread* th);