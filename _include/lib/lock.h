#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef struct {
    // 0 free / 1 locked
    volatile uint32_t slock;
} spinlock_t;

static inline void spinlock_init(spinlock_t* l)
{
    l->slock = 0;
}

typedef struct {
    uint64_t flags;
} irqlock_t;


/*
    Spinlock
*/
#define SPINLOCK_INIT {.slock = 0}

extern bool _spin_try_lock(spinlock_t* lock);
#define spin_try_lock(l) _spin_try_lock(l)

extern void _spin_lock(spinlock_t* lock);
#define spin_lock(l) _spin_lock(l)

extern void _spin_unlock(spinlock_t* lock);
#define spin_unlock(l) _spin_unlock(l)


#define spinlocked(pt_lock)                                    \
    for (__attribute((unused, cleanup(__spinlocked_defer)))    \
         spinlock_t* __spnlv = (_spin_lock(pt_lock), pt_lock); \
         __spnlv != NULL;                                      \
         __spnlv = NULL)


void __spinlocked_defer(spinlock_t** pt_lock);


/*
    irqlock
*/
/// It is not an spinlock, only disables irqs and returns the daif state
extern irqlock_t _irq_lock(void);
#define irq_lock() _irq_lock()
/// It is not an spinlock, only restores the previous state of daif
extern void _irq_unlock(irqlock_t l);
#define irq_unlock(l) _irq_unlock(l)

// locks the inside scope. Returning or breaking from this scope will not unlock
// the lock.
#define irqlocked()                                         \
    for (irqlock_t _i = {true}, _f = _irq_lock(); _i.flags; \
         _i = (_irq_unlock(_f), (irqlock_t) {false}))


void __irqlocked_defer(irqlock_t* __v);

static inline void a()
{
    for (irqlock_t
             _i = {true},
             __attribute((unused, cleanup(__irqlocked_defer))) _f = _irq_lock();
         _i.flags;
         _i = (irqlock_t) {false}) {
    }
}


/*
    spinirqlock
*/
extern irqlock_t _spin_lock_irqsave(spinlock_t* lock);

#define spin_lock_irqsave(lock) _spin_lock_irqsave(lock)

extern void _spin_unlock_irqrestore(spinlock_t* lock, irqlock_t flags);
#define spin_unlock_irqrestore(lock, flags) _spin_unlock_irqrestore(lock, flags)

#define irq_spinlocked(pt_lock)                                               \
    for (__attribute((                                                        \
             unused,                                                          \
             cleanup(__irq_spinlocked_defer))) __irq_spinlocked_defer_t __v = \
             {                                                                \
                 pt_lock,                                                     \
                 _spin_lock_irqsave((pt_lock)),                               \
             };                                                               \
         __v.lck != NULL;                                                     \
         __v.lck = NULL)


typedef struct {
    spinlock_t* lck;
    irqlock_t flags;
} __irq_spinlocked_defer_t;


void __irq_spinlocked_defer(__irq_spinlocked_defer_t* __v);


/*
    corelock
*/
typedef struct {
    uint32_t n;
    volatile uint32_t l;
} corelock_t;

void corelock_init(corelock_t* l);


/// locks other cores from entering the protected zone. The same core can access
/// the same zone as many times as needed (UINT32_MAX -1 times)
void core_lock(corelock_t* l);

/// unlocks the lock. If the core has locked it more than once, it needs to be
/// called the exact same times to unlock it.
void core_unlock(corelock_t* l);

bool core_try_lock(corelock_t* l);


#define corelocked(pt_lock)                                 \
    for (__attribute((unused, cleanup(__corelocked_defer))) \
         corelock_t* __v = (core_lock(pt_lock), pt_lock);   \
         __v != NULL;                                       \
         __v = NULL)


void __corelocked_defer(corelock_t** __v);
