#include "task.h"

#include <arm/mmu.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <lib/data_structures/kvec.h>
#include <lib/lock.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/devices/device.h"
#include "kernel/io/vfs_serial.h"
#include "kernel/smp.h"
#include "kernel/task.h"
#include "lib/data_structures/rbtree.h"


static atomic_ulong task_uid;

typedef struct thread_ref_node {
    struct thread_ref_node *next, *prev;
    thread_t*               thread_ref;
} thread_ref_node_t;

void task_ctl_init()
{
    atomic_store(&task_uid, 1);
}

task_t* task_new(const char* name)
{
    task_t* t = kmalloc(sizeof(task_t));

    *t = (task_t) {
        .task_uid            = atomic_fetch_add(&task_uid, 1),
        .name                = name,
        .state               = TASK_NEW,
        .exit_code           = 0,
        .reschedule_required = false,
        .memory_lock         = SPINLOCK_INIT,
        .mapping             = mm_mmu_mapping_new(MMU_LO),
        .regions             = RBT_INIT,
        .files               = {0},
        .threads_lock        = SPINLOCK_INIT,
        .threads_per_cpu     = {[0 ... NUM_CPUS - 1] = 0},
        .thread_ref_count    = 0,
        .threads             = NULL,
    };

    vfs_serial_bind_stdio(
        &t->files,
        device_get_primary(DEVICE_CLASS_SERIAL)->uid);

    return t;
}

static thread_ref_node_t* new_ref_node()
{
    return kmalloc(sizeof(thread_ref_node_t));
}

static inline void add_thread_ref(task_t* task, thread_t* th)
{
    DEBUG_ASSERT(th != NULL);
    DEBUG_ASSERT_TASK_IS_THREAD_LOCKED(task);

    thread_ref_node_t* node = new_ref_node();
    node->thread_ref        = th;

    if (task->threads) {
        node->next          = task->threads;
        node->prev          = NULL;
        task->threads->prev = node;
    }
    else {
        node->next = NULL;
        node->prev = NULL;
    }

    if (task->thread_ref_count == 1024)
        PANIC("TODO: Handle max task thread count reached");

    task->thread_ref_count++;
    task->threads = node;
}

void task_add_thread_ref(task_t* t, struct thread* th)
{
    irqflags_t flags = spinlock_acquire_irqsave(&t->threads_lock);

    add_thread_ref(t, th);

    spinlock_release_irqrestore(&t->threads_lock, flags);
}

void task_add_thread_refs(task_t* t, struct thread** th, size_t count)
{
    irqflags_t flags = spinlock_acquire_irqsave(&t->threads_lock);

    for (size_t i = 0; i < count; i++)
        add_thread_ref(t, th[i]);

    spinlock_release_irqrestore(&t->threads_lock, flags);
}

bool task_try_delete_thread_ref(task_t* task, struct thread* th)
{
    DEBUG_ASSERT(th != NULL);
    DEBUG_ASSERT_TASK_IS_THREAD_LOCKED(task);

    bool found = false;

    thread_ref_node_t* node = task->threads;

    while (node) {
        if (node->thread_ref != th) {
            node = node->next;
            continue;
        }

        found = true;

        if (node->prev)
            node->prev->next = node->next;
        else
            task->threads = node->next;

        if (node->next)
            node->next->prev = node->prev;

        kfree(node);
        task->thread_ref_count--;
        break;
    }

    if (task->threads == NULL) {
        if (atomic_exchange(&task->state, TASK_DEAD) == TASK_DEAD)
            PANIC();

        dbg_printf(
            DEBUG_TRACE,
            "[free_task] freed task %s with code %d",
            task->name,
            atomic_load(&task->exit_code));
    }

    return found;
}

void terminate_task(task_t* task, uint32_t exit_code)
{
    task_state_e prev_state = atomic_exchange(&task->state, TASK_DYING);
    DEBUG_ASSERT(prev_state != TASK_DEAD);

    if (prev_state != TASK_DYING)
        atomic_store(&task->exit_code, exit_code);

    dbg_printf(
        DEBUG_TRACE,
        "[terminate_task] terminated task %s with code %d",
        task->name,
        exit_code);
}

bool task_owns_thread(const task_t* task, uint64_t thid)
{
    return task_get_thread(task, thid) != NULL;
}

thread_t* task_get_thread(const task_t* task, uint64_t thid)
{
    DEBUG_ASSERT_TASK_IS_THREAD_LOCKED(task);

    thread_ref_node_t* node = task->threads;

    while (node) {
        if (node->thread_ref->th_uid == thid)
            return node->thread_ref;

        node = node->next;
    }

    return NULL;
}

#ifdef DEBUG
bool task_owner_memory_is_locked(const task_t* task)
{
    return spinlock_is_locked(&task->memory_lock);
}

bool task_owner_threads_is_locked(const task_t* task)
{
    return spinlock_is_locked(&task->threads_lock);
}
#endif
