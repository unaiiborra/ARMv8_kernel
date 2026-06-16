#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.10 - 7383

#define UART_UESC_OFFSET 0x9CUL

#define UESC_VALUE_STRUCT_NAME UartUescValue

MMIO_DECLARE_REG32_VALUE_STRUCT(UESC_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UESC, UESC_VALUE_STRUCT_NAME, UART_UESC_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UESC, UESC_VALUE_STRUCT_NAME, UART_UESC_OFFSET);

// Helper
#define UESC_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UESC,                                  \
        bf_name,                               \
        UESC_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UESC,                                  \
        bf_name,                               \
        UESC_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

// ESC_CHAR
#define ESC_CHAR_SHIFT 0
#define ESC_CHAR_MASK  (0xFF << ESC_CHAR_SHIFT)
UESC_DECLARE_BIT_FIELD_FNS(ESC_CHAR, uint8_t);
