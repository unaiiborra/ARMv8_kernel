#include "task.h"

#include <arm/mmu.h>
#include <kernel/io/stdio.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <lib/lock.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/task.h"
#include "lib/math.h"
#include "lib/stdattribute.h"

static atomic_ulong task_uid;


void task_ctl_init()
{
    atomic_store(&task_uid, 1);
}


task_t* task_new(const char* name, size_t stack_size)
{
    task_t* t = kmalloc(sizeof(task_t));

    *t = (task_t) {
        .task_uid    = atomic_fetch_add(&task_uid, 1),
        .name        = name,
        .lock        = SPINLOCK_INIT,
        .state       = TASK_NEW,
        .stack_pages = div_ceil(stack_size, PAGE_SIZE),
        .mapping     = mm_mmu_mapping_new(MMU_LO),
        .regions     = NULL,
        .threads     = kvec_new(thread*),
    };

    return t;
}


static inline bool add_thread_ref(task_t* t, struct thread* th)
{
    DEBUG_ASSERT(th != NULL);

    size_t   n       = kvec_len(&t->threads);
    thread** threads = kvec_dataT(thread*, &t->threads);

    for (size_t i = 0; i < n; i++) {
        if (threads[i] == NULL) {
            threads[i] = th;
            return true;
        }
    }

    dbgT(int64_t) idx = kvec_push(&t->threads, &th);
    DEBUG_ASSERT(idx >= 0);

    return true;
}


void task_add_thread_ref(task_t* t, struct thread* th)
{
    irqflags_t flags = spinlock_acquire_irqsave(&t->lock);

    add_thread_ref(t, th);

    spinlock_release_irqrestore(&t->lock, flags);
}


void task_add_thread_refs(task_t* t, struct thread** th, size_t count)
{
    irqflags_t flags = spinlock_acquire_irqsave(&t->lock);

    for (size_t i = 0; i < count; i++) {
        add_thread_ref(t, th[i]);
    }

    spinlock_release_irqrestore(&t->lock, flags);
}


void task_delete_thread_ref(task_t* t, struct thread* th)
{
    dbgT(bool) found = false;

    spinlocked_irqsave(&t->lock)
    {
        size_t n = kvec_len(&t->threads);

        thread** threads = kvec_dataT(thread*, &t->threads);

        for (size_t i = 0; i < n; i++) {
            if (threads[i] == th) {
                if (i == n - 1) {
                    // trim trailing NULL
                    kvec_pop(&t->threads, NULL);

                    // pop NULLs until the first valid pt
                    while (kvec_len(&t->threads) > 0) {
                        thread* last = NULL;
                        kvec_get_copy(
                            &t->threads,
                            kvec_len(&t->threads) - 1,
                            &last);

                        if (last != NULL)
                            break;

                        kvec_pop(&t->threads, NULL);
                    }
                }
                else {
                    threads[i] = NULL;
                }

                found = true;
                break;
            }
        }

        DEBUG_ASSERT(found);

        if (kvec_len(&t->threads) == 0) {
            // TODO: task delete and free
        }
    }
}


void terminate_task(task_t* task, uint32_t exit_code)
{
    atomic_store(&task->state, TASK_DYING);

    spinlocked(&task->lock)
    {
        size_t   n       = kvec_len(&task->threads);
        thread** threads = kvec_data(&task->threads);

        for (size_t i = 0; i < n; i++) {
            unschedule_thread(threads[i]);
        }
    }

    (void)exit_code;
    dbg_printf(
        DEBUG_TRACE,
        "[terminate_task] terminated task %s with code %d",
        task->name,
        exit_code);
}
