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
