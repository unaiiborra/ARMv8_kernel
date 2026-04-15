#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define __CONCAT3(x, y, z) x##y##z
#define __CONCAT(x, y, z)  __CONCAT3(x, y, z)
#define __ITER_VAR(name)   __CONCAT(__iter_, __LINE__, name)
#define __LOCK_VAR(name)   __CONCAT(__lock_, __LINE__, name)

#define __DEFER(cb) __attribute__((unused, cleanup(cb)))


/*
Spinlock
*/

typedef struct {
    volatile uint32_t slock; // 0 free / 1 locked
} spinlock_t;

#define SPINLOCK_INIT (spinlock_t) {.slock = 0}

static inline void spinlock_init(spinlock_t* l)
{
    *l = SPINLOCK_INIT;
}

extern bool _spin_try_lock(spinlock_t* lock);
extern void _spin_lock(spinlock_t* lock);
extern void _spin_unlock(spinlock_t* lock);

#define spin_try_lock(l) _spin_try_lock(l)
#define spin_lock(l)     _spin_lock(l)
#define spin_unlock(l)   _spin_unlock(l)

static inline void __spinlocked_defer(spinlock_t** lock)
{
    if (lock && *lock) {
        _spin_unlock(*lock);
    }
}

#define spinlocked(lock)                                                     \
    for (spinlock_t * __ITER_VAR(spinlocked) = (void*)1,                     \
                      __DEFER(__spinlocked_defer) * __LOCK_VAR(spinlocked) = \
                          (_spin_lock(lock), (lock));                        \
         __ITER_VAR(spinlocked);                                             \
         __ITER_VAR(spinlocked) = NULL)


/*
irqlock
*/

typedef struct {
    uint64_t flags;
} irqlock_t;

extern irqlock_t _irq_lock(void);

extern void _irq_unlock(irqlock_t l);

#define irq_lock()    _irq_lock()
#define irq_unlock(l) _irq_unlock(l)

static inline void __irqlocked_defer(irqlock_t* v)
{
    _irq_unlock(*v);
}

#define irqlocked()                                         \
    for (irqlock_t __ITER_VAR(irqlocked) = {1},             \
                   __DEFER(__irqlocked_defer)               \
                       __LOCK_VAR(irqlocked) = _irq_lock(); \
         __ITER_VAR(irqlocked).flags;                       \
         __ITER_VAR(irqlocked).flags = 0)

/*
spin + irqlock
*/


extern irqlock_t _spin_lock_irqsave(spinlock_t* lock);
extern void      _spin_unlock_irqrestore(spinlock_t* lock, irqlock_t flags);

#define spin_lock_irqsave(lock)             _spin_lock_irqsave(lock)
#define spin_unlock_irqrestore(lock, flags) _spin_unlock_irqrestore(lock, flags)


typedef struct {
    spinlock_t* lock;
    irqlock_t   irq;
} __spin_irq_guard_t;

static inline void __spin_irq_defer(__spin_irq_guard_t* v)
{
    if (v && v->lock) {
        _spin_unlock_irqrestore(v->lock, v->irq);
    }
}
#define irq_spinlocked(pt_lock)                                             \
    for (__spin_irq_guard_t                                                 \
             __ITER_VAR(irq_spinlocked) = {NULL, {true}},                   \
             __DEFER(__spin_irq_defer) __LOCK_VAR(                          \
                 irq_spinlocked) = {(pt_lock), spin_lock_irqsave(pt_lock)}; \
         __ITER_VAR(irq_spinlocked).irq.flags;                              \
         __ITER_VAR(irq_spinlocked).irq.flags = false)


/*
corelock
*/


typedef struct {
    uint32_t          n;
    volatile uint32_t l;
} corelock_t;

#define CORELOCK_INIT (corelock_t) {.l = ~(uint32_t)0, .n = 0}

static inline void corelock_init(corelock_t* l)
{
    *l = CORELOCK_INIT;
}

extern void core_lock(corelock_t* l);

extern void core_unlock(corelock_t* l);
extern bool core_try_lock(corelock_t* l);

static inline void __corelocked_defer(corelock_t** lock)
{
    if (lock && *lock) {
        core_unlock(*lock);
    }
}

#define corelocked(lock)                                                     \
    for (corelock_t * __ITER_VAR(corelocked) = (void*)1,                     \
                      __DEFER(__corelocked_defer) * __LOCK_VAR(corelocked) = \
                          (core_lock(lock), (lock));                         \
         __ITER_VAR(corelocked);                                             \
         __ITER_VAR(corelocked) = NULL)
