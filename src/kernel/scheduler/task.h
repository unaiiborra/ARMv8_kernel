#pragma once

#include <kernel/scheduler.h>
#include <kernel/smp.h>

void task_ctl_init();

/// adds the reference of the thread to the task and updates the task thread id
/// (local th uid)
void task_add_thread_ref(task_t* t, struct thread* th, cpuid_t sched_cpu);

// no batch delete for thread refs because the acutal deleting of threads is
// allways defered so it is not possible to batch delete
bool task_try_delete_thread_ref(task_t* task, struct thread* th);
