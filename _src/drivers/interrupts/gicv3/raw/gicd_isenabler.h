#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <kernel/panic.h>
#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GICD_ISENABLER_OFFSET(n) (0x100UL + (4UL * n))

static inline void
GICV3_GICD_ISENABLER_set_bit(uintptr_t base, uint32_t n, uint32_t bit)
{
    if (bit > 31)
        PANIC("GICD_ISENABLER: bit must be <= 31");

    *((reg32_ptr)(base + GICD_ISENABLER_OFFSET(n))) = (1UL << bit);
}
