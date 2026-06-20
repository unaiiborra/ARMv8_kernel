#pragma once

#include <arm/mmu.h>
#include <kernel/vfs.h>
#include <lib/data_structures/kvec.h>
#include <lib/data_structures/rbtree.h>
#include <lib/lock.h>
#include <stdatomic.h>
#include <stdint.h>

typedef struct thread          thread_t;
typedef struct thread_ref_node thread_ref_node_t;

typedef enum {
    TASK_NEW,   // just created, might not even have threads assigne
    TASK_ALIVE, // has at least 1 thread alive
    TASK_DYING, // the process called exit or aborted, defers the killing of
                // threads
    TASK_DEAD,  // task has no threads alive
} task_state_e;

typedef struct task {
    uint64_t             task_uid;
    const char*          name;
    _Atomic task_state_e state;
    atomic_int           exit_code;
    atomic_bool          reschedule_required;

    spinlock_t  memory_lock;
    mmu_mapping mapping;
    rbtree_t    regions;
    fd_table_t  files;

    spinlock_t         threads_lock;
    uint32_t           threads_per_cpu[NUM_CPUS];
    uint32_t           thread_ref_count;
    thread_ref_node_t* threads;
} task_t;

task_t*   task_new(const char* name);
void      task_delete(task_t* ut);
void      terminate_task(task_t* task, uint32_t exit_code);
bool      task_owns_thread(const task_t* task, uint64_t thid);
thread_t* task_get_thread(const task_t* task, uint64_t thid);
thread_t* task_get_any_thread_with_sched_cpu(
    const task_t* task,
    cpuid_t       sched_cpu);

#ifdef DEBUG
bool task_owner_memory_is_locked(const task_t* task);
bool task_owner_threads_is_locked(const task_t* task);
#    define DEBUG_ASSERT_TASK_IS_MEMORY_LOCKED(task) \
        DEBUG_ASSERT(                                \
            task_owner_memory_is_locked(task),       \
            "task is not memory locked!")
#    define DEBUG_ASSERT_TASK_IS_THREAD_LOCKED(task) \
        DEBUG_ASSERT(                                \
            task_owner_threads_is_locked(task),      \
            "task is not thread locked!")
#else
#    define task_owner_memory_is_locked(task)
#    define task_owner_threads_is_locked(task)
#    define DEBUG_ASSERT_TASK_IS_MEMORY_LOCKED(task)
#    define DEBUG_ASSERT_TASK_IS_THREAD_LOCKED(task)
#endif
