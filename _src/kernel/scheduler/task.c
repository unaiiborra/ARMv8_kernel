#include "task.h"

#include <arm/mmu.h>
#include <kernel/lib/kvec.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>
#include <lib/lock/spinlock.h>
#include <stdatomic.h>
#include <stddef.h>

#include "lib/math.h"


static atomic_ullong task_uid;


void task_ctl_init()
{
    atomic_store(&task_uid, 0);
}


utask* utask_new(const char* name, size_t stack_size)
{
    utask* ut = kmalloc(sizeof(utask));

    *ut = (utask) {
        .task_uid = atomic_fetch_add(&task_uid, 1),
        .task_name = name,
        .lock = {0},
        .mapping = mm_mmu_mapping_new(MMU_LO),
        .regions = NULL,
        .threads = kvec_new(thread*),
        .entry = 0x0,
        .stack_pages = DIV_CEIL(stack_size, KPAGE_SIZE),
    };

    spinlock_init(&ut->lock);

    return ut;
}


void utask_delete(utask* ut)
{
    kfree(ut);
}
