#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <kernel/panic.h>
#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GICD_ICENABLER_OFFSET(n) (0x180UL + (4UL * (n)))

static inline void
GICV3_GICD_ICENABLER_set_bit(uintptr_t base, uint32_t n, uint32_t bit)
{
    if (bit > 31)
        PANIC("GICD_ICENABLER: bit must be <= 31");

    *((reg32_ptr)(base + GICD_ICENABLER_OFFSET(n))) = (1UL << bit);
}