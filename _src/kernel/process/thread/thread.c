#include "thread.h"

#include <arm/mmu.h>
#include <drivers/arm_generic_timer/arm_generic_timer.h>
#include <kernel/hardware.h>
#include <kernel/lib/smp.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/process/process.h>
#include <kernel/process/thread.h>
#include <lib/align.h>
#include <lib/lock/_lock_types.h>
#include <lib/lock/corelock.h>
#include <lib/lock/spinlock.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>

#include "../process/process.h"


#define THREAD_CONTAINER_SIZE (KPAGE_SIZE * 2)
#define THREAD_CONTAINER_ALIGN THREAD_CONTAINER_SIZE

#define THREADS_PER_CONTAINER 254
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
static spinlock_t th_container_lock;
static uint64 th_id_counter;

static thread* local_thread[NUM_CORES];


/// returns the core local executed thread from memory, it is only needed after
/// an exception from el0, as then it is saved in sp_el0 for fast access
static inline thread* thread_get_local()
{
    return local_thread[get_core_id()];
}

static inline void thread_set_local(thread* th)
{
    local_thread[get_core_id()] = th;
}

/// saves the local core executed thread ptr for fast access in sp_el0
static inline void thread_set_current(thread* th)
{
    asm volatile("msr sp_el0, %0" : : "r"(th));
}


void pthread_ctrl_init()
{
    thread_container_list = NULL;
    th_id_counter = 0;
    spinlock_init(&th_container_lock);

#ifdef DEBUG
    for (size_t i = 0; i < NUM_CORES; i++)
        local_thread[i] = NULL;
#endif

    thread_set_current(NULL);
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

typedef enum {
    THREAD_UNLOCKED,
    THREAD_LOCKED_ACQUIRED,
    THREAD_LOCKED_UNACKQUIRED
} thread_lock_mode;

static thread* thread_new_internal(proc* owner, thread_lock_mode lock_mode)
{
    thread_ctx* ctx = kmalloc(sizeof(thread_ctx));

    ctx->owner = owner;
    ctx->th_id = __atomic_add_fetch(&th_id_counter, 1, __ATOMIC_SEQ_CST);

    thread_container *cur = thread_container_list, *prev = NULL;

    spin_lock(&th_container_lock);

    loop
    {
        while (cur) {
            for (size_t i = 0; i < THREAD_RESERVED_COUNT; i++) {
                if (cur->reserved[i] == ~(bitfield64)0)
                    continue;

                for (size_t j = 0; j < BF_BITS; j++) {
                    if (!bitfield_get(cur->reserved[i], j)) {
                        bitfield_set_high(cur->reserved[i], j);
                        thread* th = &cur->pt[i * BF_BITS + j];

                        *th = (thread) {
                            .th_flags = 0,
                            .th_state = THREAD_NEW,
                            .th_lock = {0},
                            .th_last_time_us = 0,
                            .th_ctx = ctx,
                        };

                        corelock_init(&th->th_lock);

                        switch (lock_mode) {
                            case THREAD_LOCKED_ACQUIRED: {
                                bool acquired = thread_try_acquire(th);
                                ASSERT(acquired);
                                break;
                            }
                            case THREAD_LOCKED_UNACKQUIRED:
                                core_lock(&th->th_lock);
                                break;
                            default:
                                break;
                        }

                        spin_unlock(&th_container_lock);

                        process_add_thread_ref(owner, th);

                        return th;
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

thread* thread_new(proc* owner, bool acquire)
{
    return thread_new_internal(
        owner,
        acquire ? THREAD_LOCKED_ACQUIRED : THREAD_UNLOCKED);
}

thread* thread_new_locked_unaquired(proc* owner)
{
    return thread_new_internal(owner, THREAD_LOCKED_UNACKQUIRED);
}


bool thread_delete(thread* pth)
{
    thread_ctx* ctx;

    spin_lock(&th_container_lock);

    if (pth->th_state != THREAD_DEAD) {
        spin_unlock(&th_container_lock);
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

    thread_set_current(NULL);

    spin_unlock(&th_container_lock);

    if (ctx)
        kfree(ctx);

    return true;
}


bool thread_try_acquire(thread* pth)
{
    ASSERT(pth);

    bool acquired = core_try_lock(&pth->th_lock);

    if (!acquired)
        return false;

    ASSERT(pth->th_state == THREAD_READY || pth->th_state == THREAD_NEW);

    pth->th_state = THREAD_READY;

    thread_set_current(pth);

    return true;
}


bool thread_resume()
{
    thread* pth = thread_get_current();

    if (pth == NULL)
        return false;

    proc* process = pth->th_ctx->owner;

    mmu_core_set_mapping(
        mm_mmu_core_handler_get_self(),
        &process->map.usr_mapping);

    ASSERT((pth->th_ctx->pc & KERNEL_BASE) == 0); // pc is not in high va

    thread_regs* regs = &pth->th_ctx->regs;


    pth->th_state = THREAD_RUNNING;

    thread_set_local(pth); // save the thread* in the core-local variable for
                           // later reading after exiting el0 and know what to
                           // save into sp_el0
    _thread_switch_el0(
        pth->th_ctx->pc,
        regs->ectx.fpcr,
        regs->ectx.fpsr,
        regs->sp,
        regs->ectx.x,
        regs->ectx.v);

    return true;
}


void thread_release_current()
{
    thread* th = thread_get_current();
    ASSERT(th != NULL);
    ASSERT(th->th_state == THREAD_RUNNING);

    thread_set_current(NULL);

    th->th_state = THREAD_READY;
}


void thread_userspace_save(arm_exception_ctx* ectx)
{
    thread* th = thread_get_local();
    thread_ctx* ctx = th->th_ctx;

    DEBUG_ASSERT(th->th_state == THREAD_RUNNING);

    th->th_last_time_us = AGT_cnt_time_us();

    uint64 sp, pc;
    asm volatile("mrs %0, sp_el0" : "=r"(sp));
    asm volatile("mrs %0, elr_el1" : "=r"(pc));

    ctx->regs = (thread_regs) {
        .sp = sp,
        .ectx = *ectx,
    };

    ctx->pc = pc;

    // set the pointer of the current thread for fast access as sp_el0 is not
    // being used (currently using sp_el1)
    thread_set_current(th);
}


void thread_userspace_restore(arm_exception_ctx* ectx)
{
    thread* th = thread_get_current();

    *ectx = th->th_ctx->regs.ectx;

    asm volatile("msr sp_el0, %0" : : "r"(th->th_ctx->regs.sp));
    asm volatile("msr elr_el1, %0" : : "r"(th->th_ctx->pc));
}
