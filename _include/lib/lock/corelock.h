#pragma once

#include <lib/stdbool.h>
#include <lib/stdint.h>


typedef struct {
    uint32 n;
    volatile uint32 l;
} corelock_t;

void corelock_init(corelock_t* l);


/// locks other cores from entering the protected zone. The same core can access
/// the same zone as many times as needed (UINT32_MAX -1 times)
void core_lock(corelock_t* l);

/// unlocks the lock. If the core has locked it more than once, it needs to be
/// called the exact same times to unlock it.
void core_unlock(corelock_t* l);

bool core_try_lock(corelock_t* l);


#define corelocked(lock_ptr)                                     \
    for (bool _corelocked_state = (core_lock((lock_ptr)), true); \
         _corelocked_state;                                      \
         _corelocked_state = (core_unlock((lock_ptr)), false))
