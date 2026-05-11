#pragma once

#include <arm/sysregs/sysregs.h>
#include <kernel/smp.h>
#include <stdatomic.h>




/* ---- Memory barriers ---- */

#define dsb_sy() __asm__ volatile("dsb sy" ::: "memory");
#define isb()    __asm__ volatile("isb" ::: "memory");
#define wfe()    __asm__ volatile("wfe" ::: "memory");




/* ---- Event barriers ---- */

#define sev()  __asm__ volatile("sev" ::: "memory");
#define sevl() __asm__ volatile("sevl" ::: "memory");




/* ---- Irqlock ---- */

typedef struct {
    uint32_t flags;
} irqflags_t;

[[gnu::always_inline]]
static inline irqflags_t irqsave(void)
{
    irqflags_t irqflags;
    irqflags.flags = sysreg_read(daif);

    __asm__ volatile("msr daifset, #0xf" ::: "memory");
    __asm__ volatile("isb" ::: "memory");

    return irqflags;
}

[[gnu::always_inline]]
static inline void irqrestore(irqflags_t irqflags)
{
    __asm__ volatile("isb" ::: "memory");
    sysreg_write(daif, irqflags.flags);
    __asm__ volatile("isb" ::: "memory");
}




/* ---- Spinlock ---- */

typedef struct {
    atomic_int flag;
} spinlock_t;

#define SPINLOCK_INIT (spinlock_t) {.flag = 0}

[[gnu::always_inline]] static inline void spinlock_acquire(spinlock_t* lock)
{
    int expected;

    sevl();

    do {
        do {
            wfe();
        } while (atomic_load_explicit(&lock->flag, memory_order_relaxed) != 0);

        expected = 0;
    } while (!atomic_compare_exchange_weak_explicit(
        &lock->flag,
        &expected,
        1,
        memory_order_acquire,
        memory_order_relaxed));
}

[[gnu::always_inline]] static inline void spinlock_release(spinlock_t* lock)
{
    atomic_store_explicit(&lock->flag, 0, memory_order_release);
    sev();
}

[[gnu::always_inline]] static inline bool spinlock_trylock(spinlock_t* lock)
{
    int expected = 0;
    return atomic_compare_exchange_strong_explicit(
        &lock->flag,
        &expected,
        1,
        memory_order_acquire,
        memory_order_relaxed);
}

// not for flow control as the state can change after the load
[[gnu::always_inline]] static inline bool spinlock_is_locked(spinlock_t* lock)
{
    return atomic_load_explicit(&lock->flag, memory_order_relaxed);
}




/* ---- Spinlock + Irqlock ---- */

[[gnu::always_inline]]
static inline irqflags_t spinlock_acquire_irqsave(spinlock_t* lock)
{
    irqflags_t irqflags = irqsave();
    spinlock_acquire(lock);

    return irqflags;
}

[[gnu::always_inline]]
static inline void spinlock_release_irqrestore(
    spinlock_t* lock,
    irqflags_t  irqflags)
{
    spinlock_release(lock);
    irqrestore(irqflags);
}

[[gnu::always_inline]]
static inline bool spinlock_trylock_irqsave(
    spinlock_t* lock,
    irqflags_t* irqflags)
{
    *irqflags = irqsave();

    if (spinlock_trylock(lock)) {
        return true;
    }

    irqrestore(*irqflags);
    return false;
}




/* ---- Cpulock (CPU recursive lock) ---- */

typedef struct {
    atomic_int flag;
    int        count;
} cpulock_t;

#define CPULOCK_INIT (cpulock_t) {.flag = -1, .count = 0}

[[gnu::always_inline]]
static inline void cpulock_acquire(cpulock_t* lock)
{
    cpuid_t cpuid = get_cpuid();

    if (unlikely(
            atomic_load_explicit(&lock->flag, memory_order_relaxed) ==
            (int)cpuid)) {
        lock->count++;
        return;
    }

    int expected;

    sevl();

    do {
        do {
            wfe();
        } while (atomic_load_explicit(&lock->flag, memory_order_relaxed) != -1);

        expected = -1;
    } while (!atomic_compare_exchange_weak_explicit(
        &lock->flag,
        &expected,
        (int)cpuid,
        memory_order_acquire,
        memory_order_relaxed));

    lock->count = 1;
}

[[gnu::always_inline]]
static inline void cpulock_release(cpulock_t* lock)
{
    lock->count--;

    if (lock->count == 0) {
        atomic_store_explicit(&lock->flag, -1, memory_order_release);

        sev();
    }
}

[[gnu::always_inline]]
static inline bool cpulock_trylock(cpulock_t* lock)
{
    cpuid_t cpuid = get_cpuid();

    if (atomic_load_explicit(&lock->flag, memory_order_relaxed) == (int)cpuid) {
        lock->count++;
        return true;
    }

    int  expected = -1;
    bool acquired = atomic_compare_exchange_strong_explicit(
        &lock->flag,
        &expected,
        (int)cpuid,
        memory_order_acquire,
        memory_order_relaxed);

    if (acquired) {
        lock->count = 1;
    }

    return acquired;
}


[[gnu::always_inline]]
static inline bool cpulock_is_locked(cpulock_t* lock)
{
    return atomic_load_explicit(&lock->flag, memory_order_relaxed) != -1;
}

[[gnu::always_inline]]
static inline bool cpulock_is_owner(cpulock_t* lock)
{
    return atomic_load_explicit(&lock->flag, memory_order_relaxed) ==
           (int)get_cpuid();
}



/* ---- Cpulock (CPU recursive lock) + Irqlock ---- */
// NOTE: make sure this is the required lock, use only if a recursive zone must
// be protected from access through irqs from the same core. If the idea is to
// protect from deadlock coming from irqs, use spinlock_acquire_irqsave instead

[[gnu::always_inline]]
static inline irqflags_t cpulock_acquire_irqsave(cpulock_t* lock)
{
    irqflags_t irqflags = irqsave();
    cpulock_acquire(lock);

    return irqflags;
}

[[gnu::always_inline]]
static inline void cpulock_release_irqrestore(
    cpulock_t* lock,
    irqflags_t irqflags)
{
    cpulock_release(lock);
    irqrestore(irqflags);
}

[[gnu::always_inline]]
static inline bool cpulock_trylock_irqsave(cpulock_t* lock, irqflags_t* irqflags)
{
    *irqflags = irqsave();

    if (cpulock_trylock(lock)) {
        return true;
    }

    irqrestore(*irqflags);
    return false;
}




/* ---- for like macros  ---- */
// wrap the following code of the macros with { code } and the code will be
// protected depending on the type of lock

#define __CONCAT3(x, y, z) x##y##z
#define __CONCAT(x, y, z)  __CONCAT3(x, y, z)
#define __ITER_VAR(name)   __CONCAT(__iter_, __LINE__, name)
#define __LOCK_VAR(name)   __CONCAT(__lock_, __LINE__, name)

#define __DEFER(cleanup_fn) __attribute__((unused, cleanup(cleanup_fn)))

[[gnu::always_inline]] static inline void irqrestore_cleanup(
    irqflags_t* irqflags_pt)
{
    irqrestore(*irqflags_pt);
}

[[gnu::always_inline]]
static inline void spinlock_release_cleanup(spinlock_t** lock)
{
    if (lock && *lock) {
        spinlock_release(*lock);
    }
}

[[gnu::always_inline]]
static inline void cpulock_release_cleanup(cpulock_t** lock)
{
    if (lock && *lock) {
        cpulock_release(*lock);
    }
}

typedef struct {
    spinlock_t* lock;
    irqflags_t  irqflags;
} spinlock_irqrestore_cleanup_t;

[[gnu::always_inline]]
static inline void spinlock_release_irqrestore_cleanup(
    spinlock_irqrestore_cleanup_t* cleanup)
{
    spinlock_release_irqrestore(cleanup->lock, cleanup->irqflags);
}

typedef struct {
    cpulock_t* lock;
    irqflags_t irqflags;
} cpulock_irqrestore_cleanup_t;

[[gnu::always_inline]]
static inline void cpulock_release_irqrestore_cleanup(
    cpulock_irqrestore_cleanup_t* cleanup)
{
    cpulock_release_irqrestore(cleanup->lock, cleanup->irqflags);
}


#define irqlocked()                                        \
    for (irqflags_t __ITER_VAR(irqlocked) = {0},           \
                    __DEFER(irqrestore_cleanup)            \
                        __LOCK_VAR(irqlocked) = irqsave(); \
         __ITER_VAR(irqlocked).flags < 1;                  \
         __ITER_VAR(irqlocked).flags++)

#define spinlocked(lock)                                              \
    for (spinlock_t __ITER_VAR(spinlocked) = {0},                     \
                    *__DEFER(spinlock_release_cleanup) __LOCK_VAR(    \
                        spinlocked) = (spinlock_acquire(lock), lock); \
         __ITER_VAR(spinlocked).flag < 1;                             \
         __ITER_VAR(spinlocked).flag++)

#define cpulocked(lock)                                                       \
    for (cpulock_t __ITER_VAR(cpulocked) = {0, 0},                            \
                   *__DEFER(cpulock_release_cleanup)                          \
                       __LOCK_VAR(cpulocked) = (cpulock_acquire(lock), lock); \
         __ITER_VAR(cpulocked).count < 1;                                     \
         __ITER_VAR(cpulocked).count++)

#define spinlocked_irqsave(lock)                                               \
    for (spinlock_irqrestore_cleanup_t *                                       \
             __ITER_VAR(spinlocked_irqsave) =                                  \
             (spinlock_irqrestore_cleanup_t*)0,                                \
             __DEFER(spinlock_release_irqrestore_cleanup) __LOCK_VAR(          \
                 spinlocked_irqsave) = {lock, spinlock_acquire_irqsave(lock)}; \
         (unsigned long)__ITER_VAR(spinlocked_irqsave) < 1;                    \
         __ITER_VAR(spinlocked_irqsave) = (spinlock_irqrestore_cleanup_t*)1)

#define cpulocked_irqsave(lock)                                                \
    for (cpulock_irqrestore_cleanup_t *                                        \
             __ITER_VAR(cpulocked_irqsave) = (cpulock_irqrestore_cleanup_t*)0, \
             __DEFER(cpulock_release_irqrestore_cleanup) __LOCK_VAR(           \
                 cpulocked_irqsave) = {lock, cpulock_acquire_irqsave(lock)};   \
         (unsigned long)__ITER_VAR(cpulocked_irqsave) < 1;                     \
         __ITER_VAR(cpulocked_irqsave) = (cpulock_irqrestore_cleanup_t*)1)
