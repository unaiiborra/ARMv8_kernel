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

#define GICD_ICFGR_OFFSET(n) (0xC00 + (4UL * n))

#define GICD_ICFGR_VALUE_STRUCT_NAME GicdIcfgr

MMIO_DECLARE_REG32_VALUE_STRUCT(GICD_ICFGR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER_N_OFFSET(
    GICV3,
    GICD_ICFGR,
    GICD_ICFGR_VALUE_STRUCT_NAME,
    GICD_ICFGR_OFFSET);

MMIO_DECLARE_REG32_WRITER_N_OFFSET(
    GICV3,
    GICD_ICFGR,
    GICD_ICFGR_VALUE_STRUCT_NAME,
    GICD_ICFGR_OFFSET);

static inline uint8_t
GICV3_GICD_ICFGR_get(const GICD_ICFGR_VALUE_STRUCT_NAME r, size_t slot)
{
    if (slot > 15)
        PANIC("GICD_ICFGR: slot must be <= 15");

    uint32_t shift = (slot * 2);

    return (uint8_t)((r.val >> shift) & 0x3UL);
}

static inline void
GICV3_GICD_ICFGR_set(GICD_ICFGR_VALUE_STRUCT_NAME* r, size_t slot, uint8_t v)
{
    if (slot > 15)
        PANIC("GICD_ICFGR: slot must be <= 15");
    if (v != 0b10 && v != 0b00)
        PANIC("GICD_ICFGR: value must be 0b10 or 0b00");

    uint32_t shift = (slot * 2);

    r->val = (r->val & ~(0x3UL << shift)) | (((uint32_t)v & 0x3UL) << shift);
};
