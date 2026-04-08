#include "task.h"

#include <arm/mmu.h>
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


task* task_new(const char* name, size_t stack_size)
{
    task* t = kmalloc(sizeof(task));

    *t = (task) {
        .task_uid           = atomic_fetch_add(&task_uid, 1),
        .local_thid_counter = 0,
        .name               = name,
        .lock               = SPINLOCK_INIT,
        .state              = TASK_NEW,
        .stack_pages        = DIV_CEIL(stack_size, PAGE_SIZE),
        .mapping            = mm_mmu_mapping_new(MMU_LO),
        .regions            = NULL,
        .threads            = kvec_new(thread*),
    };

    return t;
}


static void add_thread_ref(task* t, struct thread* th)
{
    DEBUG_ASSERT(th != NULL);
    DEBUG_ASSERT(
        th->local_th_uid == UNINIT_LOCAL_TH_UID,
        "add_thread_ref: thread local uid is set by add_thread_ref(), previous "
        "value must be set as UNINIT_LOCAL_TH_UID");


    thread* in = th;
    size_t  n  = kvec_len(t->threads);

    thread** threads = kvec_dataT(thread*, &t->threads);

    for (size_t i = 0; i < n; i++) {
        if (threads[i] == NULL) {
            threads[i] = in;
            return;
        }
    }

    dbgT(int64_t) idx = kvec_push(&t->threads, &in);
    DEBUG_ASSERT(idx >= 0);

    th->local_th_uid = atomic_fetch_add(&t->local_thid_counter, 1);
}


void task_add_thread_ref(task* t, struct thread* th)
{
    irqlock_t flags = spin_lock_irqsave(&t->lock);

    add_thread_ref(t, th);

    spin_unlock_irqrestore(&t->lock, flags);
}


void task_add_thread_refs(task* t, struct thread** th, size_t count)
{
    irqlock_t flags = spin_lock_irqsave(&t->lock);

    for (size_t i = 0; i < count; i++) {
        add_thread_ref(t, th[i]);
    }

    spin_unlock_irqrestore(&t->lock, flags);
}


void task_delete_thread_ref(task* t, struct thread* th)
{
    dbgT(bool) found = false;

    irq_spinlocked(&t->lock)
    {
        size_t n = kvec_len(t->threads);

        thread** threads = kvec_dataT(thread*, &t->threads);

        for (size_t i = 0; i < n; i++) {
            if (threads[i] == th) {
                if (i == n - 1) {
                    // trim trailing NULL
                    kvec_pop(&t->threads, NULL);

                    // pop NULLs until the first valid pt
                    while (kvec_len(t->threads) > 0) {
                        thread* last = NULL;
                        kvec_get_copy(
                            &t->threads,
                            kvec_len(t->threads) - 1,
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

        if (kvec_len(t->threads) == 0) {
            // TODO: task delete and free
        }
    }
}
