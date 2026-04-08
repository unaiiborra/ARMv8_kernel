#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.13 - 7384

#define UART_UBMR_OFFSET 0xA8UL

#define UBMR_VALUE_STRUCT_NAME UartUbmrValue

MMIO_DECLARE_REG32_VALUE_STRUCT(UBMR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UBMR, UBMR_VALUE_STRUCT_NAME, UART_UBMR_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UBMR, UBMR_VALUE_STRUCT_NAME, UART_UBMR_OFFSET);

// Helper
#define UBMR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UBMR,                                  \
        bf_name,                               \
        UBMR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UBMR,                                  \
        bf_name,                               \
        UBMR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

#define MOD_SHIFT 0
#define MOD_MASK  (0xFFFF << MOD_SHIFT)
UBMR_DECLARE_BIT_FIELD_FNS(MOD, uint16_t);
