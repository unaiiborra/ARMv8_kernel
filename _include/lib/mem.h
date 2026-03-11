#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/panic.h"

// phys address uintptr_t
typedef uintptr_t p_uintptr_t;

// virt address uintptr_t
typedef uintptr_t v_uintptr_t;


typedef struct {
    p_uintptr_t pa;
    v_uintptr_t va;
} pv_ptr;

static inline pv_ptr pv_ptr_new(p_uintptr_t pa, v_uintptr_t va)
{
    return (pv_ptr) {pa, va};
}

#define uintptr_t_p_to_ptr(type, uintptr_t_phys) ((type*)(uintptr_t_phys))
#define PTR_TO_UINTPTR_P(ptr) ((p_uintptr_t)(ptr))

#define UINTPTR_V_TO_PTR(type, uintptr_t_virt) ((type*)(uintptr_t_virt))
#define PTR_TO_UINTPTR_V(ptr) ((v_uintptr_t)(ptr))


size_t pa_supported_bits();


static inline bool
address_is_valid(uintptr_t a, size_t bits, bool sign_extended)
{
    ASSERT(bits > 0 && bits <= 64);

    if (bits == 64)
        return true;

    uint64_t upper_mask = ~((1ULL << bits) - 1);

    if (!sign_extended)
        return (upper_mask & a) == 0;

    uint64_t sign_bit = 1ULL << (bits - 1);

    return (a & sign_bit) ? (a & upper_mask) == upper_mask
                          : (a & upper_mask) == 0;
}

static inline v_uintptr_t va_sign_extend(v_uintptr_t va, size_t bits)
{
    if (bits == 64)
        return va;

    ASSERT(bits > 0 && bits < 64);
    ASSERT(address_is_valid(va, bits, false));

    uint64_t sign_bit = 1ULL << (bits - 1);
    uint64_t mask = ~((1ULL << bits) - 1);

    v_uintptr_t a = (va & sign_bit) ? (va | mask) : va;

    DEBUG_ASSERT(address_is_valid(a, bits, true));

    return a;
}

static inline v_uintptr_t va_zero_extend(v_uintptr_t va, size_t bits)
{
    if (bits == 64)
        return va;

    ASSERT(bits > 0 && bits < 64);
    ASSERT(address_is_valid(va, bits, true));

    v_uintptr_t a = va & ((1ULL << bits) - 1);

    DEBUG_ASSERT(address_is_valid(a, bits, false));

    return a;
}


/*
 *  Mem ctrl fns
 */

// TODO: maybe doing a c wrapper makes sense for checking if simd is enabled at
// lower EL levels
#define memcpy(dst, src, size) _memcpy(dst, src, size)

/// Standard memcpy, requieres simd instructions to be enabled
extern void* _memcpy(void* dst, const void* src, size_t size);

/// Panics: if the size is not divisible by 64
void* memcpy64(void* dst, const void* src, size_t size);

/// Panics: if the addreses are not aligned to 16 bytes or the size is not
/// divisible by 64
void* memcpy64_aligned(void* dst, const void* src, size_t size);

#ifdef TEST
void test_memcpy(size_t size_start);
#endif


/// zeroes from dst to dst + size
extern void* _memzero(void* dst, size_t size);


/// UNSAFE: requires the dst to be aligned to 16 and the size to be exactly a
/// multiple of 64 or it will hang in DEBUG or cause ub in RELEASE
extern void* _memzero64(void* dst16, size_t size64);


#define memzero(dst, size) _memzero(dst, size)
#define memzero64(dst, size) _memzero64(dst, size)
