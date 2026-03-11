#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <kernel/panic.h>
#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gicv3_macros.h"

#define GICR_IPRIORITYR_OFFSET(n) (0x400UL + (4UL * n))

#define GICR_IPRIORITYR_VALUE_STRUCT_NAME GicrIpriorityr

MMIO_DECLARE_REG32_VALUE_STRUCT(GICR_IPRIORITYR_VALUE_STRUCT_NAME);

static inline GICR_IPRIORITYR_VALUE_STRUCT_NAME
GICV3_GICR_IPRIORITYR_read(uintptr_t base, size_t rd, size_t n)
{
    return (GICR_IPRIORITYR_VALUE_STRUCT_NAME) {
        .val = *(
            (reg32_ptr)(GICV3_SGI_BASE(base, rd) + GICR_IPRIORITYR_OFFSET(n)))};
}

static inline void GICV3_GICR_IPRIORITYR_write(
    uintptr_t base,
    size_t rd,
    size_t n,
    GICR_IPRIORITYR_VALUE_STRUCT_NAME v)
{
    *((reg32_ptr)(GICV3_SGI_BASE(base, rd) + GICR_IPRIORITYR_OFFSET(n))) =
        v.val;
}

static inline uint8_t GICV3_GICR_IPRIORITYR_BF_get(
    const GICR_IPRIORITYR_VALUE_STRUCT_NAME r,
    size_t byte_idx)
{
    if (byte_idx > 3)
        PANIC("GICR_IPRIORITYR: byte_idx index must be <= 3");

    uint32_t shift = (byte_idx * 8);

    return (uint8_t)((r.val >> shift) & 0xFFUL);
}

static inline void GICV3_GICR_IPRIORITYR_BF_set(
    GICR_IPRIORITYR_VALUE_STRUCT_NAME* r,
    size_t byte_idx,
    uint8_t priority)
{
    if (byte_idx > 3)
        PANIC("GICR_IPRIORITYR: byte_idx index must be <= 3");

    uint32_t shift = (byte_idx * 8);

    r->val = (r->val & ~(0xFFUL << shift)) |
             (((uint32_t)priority & 0xFFUL) << shift);
};
