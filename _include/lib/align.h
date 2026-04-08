#pragma once


#include <kernel/panic.h>
#include <lib/math.h>
#include <stdbool.h>
#include <stdint.h>


static inline uintptr_t align_up(uintptr_t x, size_t a)
{
    DEBUG_ASSERT(is_pow2(a), "can only align powers of 2");
    return (x + a - 1) & ~(a - 1);
}

static inline uintptr_t align_down(uintptr_t x, size_t a)
{
    DEBUG_ASSERT(is_pow2(a), "can only align powers of 2");
    return x & ~(a - 1);
}

#define align_up_pt(x, a)   ((void*)align_up((uintptr_t)(x), a))
#define align_down_pt(x, a) ((void*)align_down((uintptr_t)(x), a))


static inline bool is_aligned(uintptr_t x, size_t a)
{
    DEBUG_ASSERT(is_pow2(a), "can only align powers of 2");
    return (x & (a - 1)) == 0;
}
