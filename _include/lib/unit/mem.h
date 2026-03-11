#pragma once

#include <stddef.h>
#include <stdint.h>

#define MEM_BYTE 0x1ULL
#define MEM_KiB (MEM_BYTE * 0x400ULL)
#define MEM_MiB (MEM_KiB * 0x400ULL)
#define MEM_GiB (MEM_MiB * 0x400ULL)
#define MEM_TiB (MEM_GiB * 0x400ULL)


#define BYTES_TO_BITS(byte_n) (byte_n * 8)


static inline size_t mem_byte_to_kib(size_t byte)
{
    return byte >> 10;
}

static inline size_t mem_kib_to_byte(size_t kib)
{
    return kib << 10;
}

static inline size_t mem_kib_to_mib(size_t kib)
{
    return kib >> 10;
}

static inline size_t mem_mib_to_kib(size_t mib)
{
    return mib << 10;
}

static inline size_t mem_mib_to_gib(size_t mib)
{
    return mib >> 10;
}

static inline size_t mem_gib_to_mib(size_t gib)
{
    return gib << 10;
}

static inline size_t mem_byte_to_mib(size_t byte)
{
    return byte >> 20;
}

static inline size_t mem_mib_to_byte(size_t mib)
{
    return mib << 20;
}

static inline size_t mem_byte_to_gib(size_t byte)
{
    return byte >> 30;
}

static inline size_t mem_gib_to_byte(size_t gib)
{
    return gib << 30;
}

static inline size_t mem_kib_to_gib(size_t kib)
{
    return kib >> 20;
}

static inline size_t mem_gib_to_kib(size_t gib)
{
    return gib << 20;
}
