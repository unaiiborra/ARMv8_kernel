#include <kernel/process/pthread.h>

#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/align.h"
#include "lib/lock/_lock_types.h"
#include "lib/lock/spinlock.h"
#include "lib/stdbitfield.h"
#include "lib/stdint.h"
#include "lib/stdmacros.h"

#define THREAD_CONTAINER_SIZE (KPAGE_SIZE * 2)

#define THREADS_PER_CONTAINER 338
#define THREAD_RESERVED_COUNT BITFIELD_COUNT_FOR(THREADS_PER_CONTAINER, uint64)

typedef struct thread_container {
    struct thread_container* prev;
    struct thread_container* next;
    bitfield64 reserved[THREAD_RESERVED_COUNT];
    pthread pt[THREADS_PER_CONTAINER];
} thread_container;
_Static_assert(sizeof(thread_container) <= THREAD_CONTAINER_SIZE);


static thread_container* thread_container_list;
static spinlock_t th_lock;

static thread_container* container_new(thread_container* prev)
{
    thread_container* new_container =
        raw_kmalloc(2, "pthread container", &RAW_KMALLOC_DYNAMIC_CFG);

    DEBUG_ASSERT(is_aligned((uintptr)new_container, THREAD_CONTAINER_SIZE));


    memzero(new_container, sizeof(*new_container));

    new_container->prev = prev;
    new_container->next = NULL;

    if (prev) {
        DEBUG_ASSERT(prev->next == NULL);
        prev->next = new_container;
    }
    else {
        DEBUG_ASSERT(thread_container_list == NULL);
        thread_container_list = new_container;
    }

    return new_container;
}

static void container_try_delete(thread_container* cont)
{
    if (cont == thread_container_list)
        return;

    DEBUG_ASSERT(cont->prev != NULL);

    for (size_t i = 0; i < THREAD_RESERVED_COUNT; i++)
        if (cont->reserved[i] != 0)
            return;

    cont->prev->next = cont->next;

    raw_kfree(cont);
}


pthread* pthread_new(struct proc* owner, v_uintptr entry, size_t stack_pages)
{
    thread_container *cur, *prev;
    cur = thread_container_list;

    spin_lock(&th_lock);
find:
    while (cur) {
        for (size_t i = 0; i < THREAD_RESERVED_COUNT; i++) {
            if (cur->reserved[i] == ~(bitfield64)0)
                continue;

            for (size_t j = 0; j < BITFIELD_CAPACITY(bitfield64); j++) {
                if (!bitfield_get(cur->reserved[i], j)) {
                    bitfield_set_high(cur->reserved[i], j);

                    cur->pt[i * BITFIELD_CAPACITY(bitfield64) + j] = (pthread) {
                        .th_flags = 0,
                        .th_state = THREAD_NEW,
                        .th_last_ns = 0,
                        .th_ctx = NULL,
                    };

                    spin_unlock(&th_lock);

                    return &cur->pt[i * BITFIELD_CAPACITY(bitfield64) + j];
                }
            }

            PANIC();
        }

        prev = cur;
        cur = cur->next;
    }

    cur = container_new(prev);

    goto find;
}

void pthread_delete(thread_ctx* pth)
{
    spin_lock(&th_lock);

    

    spin_unlock(&th_lock);
}
