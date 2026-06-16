#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <kernel/panic.h>
#include <lib/mmio/mmio_macros.h>
#include <stdint.h>

#include "gicv3_macros.h"

#define GICR_ICFGR0_OFFSET 0x0C00UL // SGIs (0-15)
#define GICR_ICFGR1_OFFSET 0x0C04UL // PPIs (16-31)

typedef struct {
    uint32_t val;
} GicrIcfgr;

// GICR_ICFGR0 — SGIs (intids 0-15), writes ignored
static inline GicrIcfgr GICV3_GICR_ICFGR0_read(uintptr_t base, uint32_t cpu)
{
    return (GicrIcfgr) {
        .val = *((reg32_ptr)(GICV3_SGI_BASE(base, cpu) + GICR_ICFGR0_OFFSET))};
}

static inline void
GICV3_GICR_ICFGR0_write(uintptr_t base, uint32_t cpu, GicrIcfgr r)
{
    *((reg32_ptr)(GICV3_SGI_BASE(base, cpu) + GICR_ICFGR0_OFFSET)) = r.val;
}

// GICR_ICFGR1 — PPIs (intids 16-31)
static inline GicrIcfgr GICV3_GICR_ICFGR1_read(uintptr_t base, uint32_t cpu)
{
    return (GicrIcfgr) {
        .val = *((reg32_ptr)(GICV3_SGI_BASE(base, cpu) + GICR_ICFGR1_OFFSET))};
}

static inline void
GICV3_GICR_ICFGR1_write(uintptr_t base, uint32_t cpu, GicrIcfgr r)
{
    *((reg32_ptr)(GICV3_SGI_BASE(base, cpu) + GICR_ICFGR1_OFFSET)) = r.val;
}


static inline void
GICV3_GICR_ICFGR_set(GicrIcfgr* r, uint32_t slot, uint32_t val)
{
    if (slot > 15)
        PANIC("GICR_ICFGR: slot must be <= 15");

    r->val &= ~(0b11UL << (slot * 2));
    r->val |= (val << (slot * 2));
}