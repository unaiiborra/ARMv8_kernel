#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>

// 17.2.14.2 - 7366

#define UART_UTXD_OFFSET 0x40UL

static inline void UART_UTXD_write(uintptr_t base, uint8_t v)
{
    *((reg32_ptr)(base + (0x40UL))) = v;
}
