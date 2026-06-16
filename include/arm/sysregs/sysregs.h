#pragma once

#include <stddef.h>
#include <stdint.h>


#define sysreg_read(r)                             \
    ({                                             \
        uint64_t __val;                            \
        asm volatile("mrs %0, " #r : "=r"(__val)); \
        __val;                                     \
    })


#define sysreg_write(r, v)                               \
    do {                                                 \
        uint64_t __val = (uint64_t)(v);                  \
        asm volatile("msr " #r ", %x0" : : "rZ"(__val)); \
    } while (0)
