#include <kernel/panic.h>
#include <lib/mem.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>

extern void* _memcpy64(void* dst, const void* src, size_t size);
extern void* _memcpy32(void* dst, const void* src, size_t size);
extern void* _memcpy16(void* dst, const void* src, size_t size);
extern void* _memcpy8(void* dst, const void* src, size_t size);
extern void* _memcpy4(void* dst, const void* src, size_t size);
extern void* _memcpy1(void* dst, const void* src, size_t size);

// byte per byte
// extern void *_memcpy(void *dst, void *src, size_t size);

void* memcpy64_aligned(void* dst, const void* src, size_t size)
{
    if (size == 0)
        return dst;

    if (((uintptr_t)dst & 15) != 0)
        PANIC("memcpy64_aligned: dst not aligned to 16 bytes");

    if (((uintptr_t)src & 15) != 0)
        PANIC("memcpy64_aligned: src not aligned to 16 bytes");

    if ((size & 63) != 0)
        PANIC("memcpy64_aligned: size is not a multiple of 64");

    return _memcpy64(dst, src, size);
}

void* memcpy64(void* dst, const void* src, size_t size)
{
    if (size == 0)
        return dst;

    if ((size & 63) != 0)
        PANIC("memcpy64: size is not a multiple of 64");

    return _memcpy64(dst, src, size);
}


#ifdef TEST

#    include "kernel/io/stdio.h"
#    include "lib/stdmacros.h"

#    define MEMCPY_TEST_SIZE 1048576 * 4

_Alignas(64) uint8_t src[MEMCPY_TEST_SIZE];
_Alignas(64) uint8_t dst[MEMCPY_TEST_SIZE];
_Alignas(64) uint8_t ref[MEMCPY_TEST_SIZE];

void test_memcpy(size_t size_start)
{
    char buf[100];

    for (size_t i = 0; i < sizeof(src); i++)
        src[i] = (uint8_t)(i * 37 + 13);

    for (size_t i = size_start; i < sizeof(src); i++) {
        for (size_t j = 0; j < sizeof(src); j++)
            dst[j] = (uint8_t)(0xFF);

        memcpy(&dst[sizeof(dst) - 1 - i], &src[sizeof(src) - 1 - i], i);

        for (size_t j = 0; j < i; j++) {
            if (dst[(sizeof(dst) - 1 - i) + j] !=
                src[(sizeof(src) - 1 - i) + j]) {
                kprint("Something went wrong");

                loop
                {
                }
            }
        }

        if (i % 10000 == 0) {
            kprint("i: ");
            kprint(stdint_to_ascii(
                (STDINT_UNION) {.int64 = i},
                STDINT_UINT64,
                buf,
                100,
                STDINT_BASE_REPR_DEC));
            kprint(" ok\n\r");
        }
    }

    kprint("FINISHED without hang");
}
#endif
