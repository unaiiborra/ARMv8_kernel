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

#define GICD_IGROUPR_OFFSET(n) (0x80UL + (4UL * n))

#define GICD_IGROUPR_VALUE_STRUCT_NAME GicdIgroupr

MMIO_DECLARE_REG32_VALUE_STRUCT(GICD_IGROUPR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER_N_OFFSET(
    GICV3,
    GICD_IGROUPR,
    GICD_IGROUPR_VALUE_STRUCT_NAME,
    GICD_IGROUPR_OFFSET);

MMIO_DECLARE_REG32_WRITER_N_OFFSET(
    GICV3,
    GICD_IGROUPR,
    GICD_IGROUPR_VALUE_STRUCT_NAME,
    GICD_IGROUPR_OFFSET);

static inline bool
GICV3_GICD_IGROUPR_BF_get(const GICD_IGROUPR_VALUE_STRUCT_NAME r, uint32_t bit)
{
    if (bit > 31)
        PANIC("GICD_IGROUPR: bit index must be <= 31");

    return (bool)((r.val >> bit) & 1UL);
}

static inline void GICV3_GICD_IGROUPR_BF_set(
    GICD_IGROUPR_VALUE_STRUCT_NAME* r,
    uint32_t                        bit,
    bool                            v)
{
    if (bit > 31)
        PANIC("GICD_IGROUPR: bit index must be <= 31");

    r->val = (r->val & ~(1UL << bit)) | (((uint32_t)v & 1UL) << bit);
};
