#pragma once

#include <stddef.h>

#define MEM_BYTE(n) (0x1ULL * (n))
#define MEM_KiB(n)  (MEM_BYTE(1) * 0x400ULL * (n))
#define MEM_MiB(n)  (MEM_KiB(1) * 0x400ULL * (n))
#define MEM_GiB(n)  (MEM_MiB(1) * 0x400ULL * (n))
#define MEM_TiB(n)  (MEM_GiB(1) * 0x400ULL * (n))


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
