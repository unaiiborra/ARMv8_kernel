#include "thread.h"

#include <arm/mmu.h>
#include <kernel/hardware.h>
#include <kernel/lib/smp.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/process/thread.h>
#include <lib/align.h>
#include <lib/lock/_lock_types.h>
#include <lib/lock/spinlock.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>

#include "../process/process.h"


#define THREAD_CONTAINER_SIZE (KPAGE_SIZE * 2)
#define THREAD_CONTAINER_ALIGN THREAD_CONTAINER_SIZE

#define THREADS_PER_CONTAINER 338
#define THREAD_RESERVED_COUNT BITFIELD_COUNT_FOR(THREADS_PER_CONTAINER, uint64)
#define BF_BITS BITFIELD_CAPACITY(bitfield64)

typedef struct thread_container {
    struct thread_container* prev;
    struct thread_container* next;
    bitfield64 reserved[THREAD_RESERVED_COUNT];
    thread pt[THREADS_PER_CONTAINER];
} thread_container;
_Static_assert(sizeof(thread_container) <= THREAD_CONTAINER_SIZE);


static thread_container* thread_container_list;
static spinlock_t th_lock;
static uint64 th_id_counter;


static thread* local_thread[NUM_CORES];


void pthread_ctrl_init()
{
    thread_container_list = NULL;
    th_id_counter = 0;
    spinlock_init(&th_lock);

    for (size_t i = 0; i < NUM_CORES; i++)
        local_thread[i] = NULL;
}


static thread_container* container_new(thread_container* prev)
{
    thread_container* new_container =
        raw_kmalloc(2, "pthread container", &RAW_KMALLOC_DYNAMIC_CFG);

    DEBUG_ASSERT(is_aligned((uintptr)new_container, THREAD_CONTAINER_ALIGN));


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


thread* thread_new(proc* owner)
{
    thread_ctx* ctx = kmalloc(sizeof(thread_ctx));

    ctx->owner = owner;
    ctx->th_id = __atomic_add_fetch(&th_id_counter, 1, __ATOMIC_SEQ_CST);

    thread_container *cur, *prev;
    cur = thread_container_list;
    prev = NULL;

    spin_lock(&th_lock);

    loop
    {
        while (cur) {
            for (size_t i = 0; i < THREAD_RESERVED_COUNT; i++) {
                if (cur->reserved[i] == ~(bitfield64)0)
                    continue;

                for (size_t j = 0; j < BF_BITS; j++) {
                    if (!bitfield_get(cur->reserved[i], j)) {
                        bitfield_set_high(cur->reserved[i], j);

                        cur->pt[i * BF_BITS + j] = (thread) {
                            .th_flags = 0,
                            .th_state = THREAD_NEW,
                            .th_last_ns = 0,
                            .th_ctx = ctx,
                        };

                        spin_unlock(&th_lock);

                        process_add_thread_ref(owner, &cur->pt[i * BF_BITS + j]);

                        return &cur->pt[i * BF_BITS + j];
                    }
                }

                PANIC();
            }

            prev = cur;
            cur = cur->next;
        }

        cur = container_new(prev);
    }
}


bool thread_delete(thread* pth)
{
    thread_ctx* ctx;

    spin_lock(&th_lock);

    if (pth->th_state != THREAD_DEAD) {
        spin_unlock(&th_lock);
        return false;
    }

    ctx = pth->th_ctx;

    thread_container* container = align_down_ptr(pth, THREAD_CONTAINER_ALIGN);
    thread* base = &container->pt[0];

    size_t i = pth - base;

    DEBUG_ASSERT(&container->pt[i] == pth);

    pth->th_flags = 0;

    process_remove_thread_ref(ctx->owner, pth);

    bitfield_clear(container->reserved[i / BF_BITS], i % BF_BITS);

    container_try_delete(container);

    local_thread[get_core_id()] = NULL;

    spin_unlock(&th_lock);

    if (ctx)
        kfree(ctx);

    return true;
}


bool thread_resume(thread* pth)
{
    proc* process = pth->th_ctx->owner;

    spin_lock(&th_lock);

    mmu_core_set_mapping(
        mm_mmu_core_handler_get_self(),
        &process->map.usr_mapping);

    spin_unlock(&th_lock);

    ASSERT((pth->th_ctx->pc & KERNEL_BASE) == 0); // pc is not in high va

    thread_regs* regs = &pth->th_ctx->regs;

    local_thread[get_core_id()] = pth;

    _thread_switch_el0(
        pth->th_ctx->pc,
        regs->fpcr,
        regs->fpsr,
        regs->sp,
        regs->gpr,
        regs->vec);

    return true;
}


thread* thread_get_local()
{
    return local_thread[get_core_id()];
}