#include <kernel/panic.h>
#include <lib/lock.h>


#define UNLOCKED_VALUE ~(uint32_t)0

extern void _core_lock(volatile uint32_t* l);
extern void _core_unlock(volatile uint32_t* l);
extern bool _core_try_lock(volatile uint32_t* l);


void core_lock(corelock_t* l)
{
    _core_lock(&l->l);

    __atomic_add_fetch(&l->n, 1, __ATOMIC_SEQ_CST);
}


void core_unlock(corelock_t* l)
{
    ASSERT(l->l != UNLOCKED_VALUE, "core_unlock: was already unlocked");

    ASSERT(l->n > 0);


    if (--l->n == 0) {
        _core_unlock(&l->l);
        DEBUG_ASSERT(l->l == ~(uint32_t)0 && l->n == 0);
    }
}


bool core_try_lock(corelock_t* l)
{
    bool taken = _core_try_lock(&l->l);

    if (!taken)
        return false;

    l->n++;
    return true;
}
