#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline uint32_t log2_floor_u32(uint32_t x)
{
    return x ? 31u - __builtin_clz(x) : 0;
}

static inline uint64_t log2_floor_u64(uint64_t x)
{
    return x ? 63u - __builtin_clzll(x) : 0;
}

static inline uint32_t log2_ceil_u32(uint32_t x)
{
    return x <= 1 ? 0 : 32u - __builtin_clz(x - 1);
}

static inline uint64_t log2_ceil_u64(uint64_t x)
{
    return x <= 1 ? 0 : 64u - __builtin_clzll(x - 1);
}

#define log2_floor(x) \
    _Generic((x), uint32_t: log2_floor_u32, uint64_t: log2_floor_u64)(x)

#define log2_ceil(x)             \
    _Generic(                    \
        (x),                     \
        uint32_t: log2_ceil_u32, \
        uint64_t: log2_ceil_u64, \
        size_t: log2_ceil_u64)(x)


#define power_of2(x) ((x) < 64 ? (1ULL << (x)) : 0)


static inline uint64_t square(uint64_t x)
{
    return x * x;
}


static inline uint64_t max(uint64_t a, uint64_t b)
{
    return a > b ? a : b;
}


static inline uint64_t min(uint64_t a, uint64_t b)
{
    return a < b ? a : b;
}


#define DIV_CEIL(a, b) ((a + b - 1) / b)

static inline uint64_t div_ceil(uint64_t a, uint64_t b)
{
    return DIV_CEIL(a, b);
}


static inline bool is_pow2(uint64_t x)
{
    return x && ((x & (x - 1)) == 0);
}


// https://jameshfisher.com/2018/03/30/round-up-power-2/
static inline uint32_t next_pow2_u32(uint32_t x)
{
    return x <= 1 ? 1u : 1u << (32 - __builtin_clz(x - 1));
}


static inline uint64_t next_pow2_u64(uint64_t x)
{
    return x <= 1 ? 1ull : 1ull << (64 - __builtin_clzll(x - 1));
}

#define next_pow2(x) \
    _Generic((x), uint32_t: next_pow2_u32, uint64_t: next_pow2_u64)(x)
