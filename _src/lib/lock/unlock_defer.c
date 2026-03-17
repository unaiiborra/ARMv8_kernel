#include <lib/lock.h>


void __spinlocked_defer(spinlock_t** pt_lock)
{
    _spin_unlock(*pt_lock);
}


void __irqlocked_defer(irqlock_t* __v)
{
    _irq_unlock(*__v);
}


void __irq_spinlocked_defer(__irq_spinlocked_defer_t* __v)
{
    _spin_unlock_irqrestore(__v->lck, __v->flags);
}


void __corelocked_defer(corelock_t** __v)
{
    core_unlock(*__v);
}
