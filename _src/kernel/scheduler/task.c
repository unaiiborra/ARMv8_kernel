#include "task.h"

#include <arm/mmu.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <lib/lock.h>
#include <stdatomic.h>
#include <stddef.h>

#include "kernel/task.h"
#include "lib/math.h"


static atomic_ulong task_uid;


void task_ctl_init()
{
    atomic_store(&task_uid, 1);
}


task* task_new(const char* name, size_t stack_size)
{
    task* t = kmalloc(sizeof(task));

    *t = (task) {
        .task_uid    = atomic_fetch_add(&task_uid, 1),
        .name        = name,
        .lock        = {0},
        .state       = TASK_NEW,
        .stack_pages = DIV_CEIL(stack_size, KPAGE_SIZE),
        .mapping     = mm_mmu_mapping_new(MMU_LO),
        .regions     = NULL,
        .threads     = kvec_new(thread*),
    };

    spinlock_init(&t->lock);

    return t;
}


void task_delete(task* t)
{
    kfree(t);
}
