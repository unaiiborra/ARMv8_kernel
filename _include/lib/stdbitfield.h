#pragma once

#include <lib/stdmacros.h>

/* Standard bitfield types */

/// Standard 8 bit bitfield
typedef unsigned char bitfield8;
/// Standard 16 bit bitfield
typedef unsigned short bitfield16;
/// Standard 32 bit bitfield
typedef unsigned int bitfield32;
/// Standard 64 bit bitfield
typedef unsigned long long bitfield64;


#define BITFIELD_CAPACITY(bf_type) TYPE_BIT_SIZE(bf_type)
#define BITFIELD_COUNT_FOR(N, bf_type) \
    (((N) + BITFIELD_CAPACITY(bf_type) - 1) / BITFIELD_CAPACITY(bf_type))




#define bitfield_get(bf, bit_n) (((bf) >> (bit_n)) & (typeof(bf))1)


#define bitfield_set_high(bf, bit_n) ((bf) |= ((typeof(bf))1 << (bit_n)))


#define bitfield_clear(bf, bit_n) ((bf) &= ~((typeof(bf))1 << (bit_n)))


#define bitfield_toggle(bf, bit_n) ((bf) ^= ((typeof(bf))1 << (bit_n)))


#define bitfield_set(bf, bit_n, v)              \
    ((bf) =                                     \
         ((bf) & ~((typeof(bf))1 << (bit_n))) | \
         ((((typeof(bf))(v)) & (typeof(bf))1) * ((typeof(bf))1 << (bit_n))))
