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

#define GICR_ICENABLER_OFFSET 0x180UL

static inline void
GICV3_GICR_ICENABLER0_set_bit(uintptr_t base, uint32_t cpu, uint32_t intid)
{
    if (intid > 31)
        PANIC("GICR_ICENABLER0: bit must be <= 31");

    *((reg32_ptr)(GICV3_SGI_BASE(base, cpu) + GICR_ICENABLER_OFFSET)) =
        (1UL << intid);
}