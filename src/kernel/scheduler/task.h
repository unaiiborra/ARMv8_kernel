#pragma once

#include <kernel/scheduler.h>

void task_ctl_init();

/// adds the reference of the thread to the task and updates the task thread id
/// (local th uid)
void task_add_thread_ref(task_t* t, struct thread* th);

/// adds the reference of the threads to the task and updates the task thread id
/// (local th uid)
void task_add_thread_refs(task_t* t, struct thread** th, size_t count);

void task_delete_thread_ref(task_t* t, struct thread* th);
// no batch delete for thread refs because the acutal deleting of threads is
// allways defered so it is not possible to batch delete