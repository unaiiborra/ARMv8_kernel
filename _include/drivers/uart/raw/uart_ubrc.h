#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.14 - 7385

#define UART_UBRC_OFFSET 0xACUL

#define UBRC_VALUE_STRUCT_NAME UartUbrcValue

MMIO_DECLARE_REG32_VALUE_STRUCT(UBRC_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UBRC, UBRC_VALUE_STRUCT_NAME, UART_UBRC_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UBRC, UBRC_VALUE_STRUCT_NAME, UART_UBRC_OFFSET);

// Helper
#define UBRC_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UBRC,                                  \
        bf_name,                               \
        UBRC_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UBRC,                                  \
        bf_name,                               \
        UBRC_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

#define BCNT_SHIFT 0
#define BCNT_MASK (0xFFFF << BCNT_SHIFT)
UBRC_DECLARE_BIT_FIELD_FNS(BCNT, bool);
