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

#define GICR_IGROUPR0_OFFSET 0x80UL

#define GICR_IGROUPR0_VALUE_STRUCT_NAME GicrIgroupr0

MMIO_DECLARE_REG32_VALUE_STRUCT(GICR_IGROUPR0_VALUE_STRUCT_NAME);

static inline GICR_IGROUPR0_VALUE_STRUCT_NAME
GICV3_GICR_IGROUPR0_read(uintptr_t base, size_t n)
{
    return (GICR_IGROUPR0_VALUE_STRUCT_NAME) {
        .val =
            *((reg32_ptr)(GICV3_SGI_BASE(base, n) + (GICR_IGROUPR0_OFFSET)))};
}

static inline void GICV3_GICR_IGROUPR0_write(
    uintptr_t                       base,
    size_t                          n,
    GICR_IGROUPR0_VALUE_STRUCT_NAME v)
{
    *((reg32_ptr)(GICV3_SGI_BASE(base, n) + (GICR_IGROUPR0_OFFSET))) = v.val;
}

/* Helpers */
static inline void
GICV3_GICR_IGROUPR0_set_bit(GICR_IGROUPR0_VALUE_STRUCT_NAME* r, size_t n, bool v)
{
#ifdef TEST
    if (n >= 32)
        PANIC("Invalid GICR_IGROUPR0 bit");
#endif

    if (v)
        r->val |= (1u << n);
    else
        r->val &= ~(1u << n);
}

static inline bool
GICV3_GICR_IGROUPR0_get_bit(GICR_IGROUPR0_VALUE_STRUCT_NAME r, size_t n)
{
#ifdef TEST
    if (n >= 32)
        PANIC("Invalid GICR_IGROUPR0 bit");
#endif

    return (r.val >> n) & 0x1u;
}
