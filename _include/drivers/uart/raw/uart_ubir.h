#pragma once


#ifndef DRIVERS
#    include <stdint.h>
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.12 - 7384

#define UART_UBIR_OFFSET 0xA4UL

#define UBIR_VALUE_STRUCT_NAME UartUbirValue

MMIO_DECLARE_REG32_VALUE_STRUCT(UBIR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UBIR, UBIR_VALUE_STRUCT_NAME, UART_UBIR_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UBIR, UBIR_VALUE_STRUCT_NAME, UART_UBIR_OFFSET);

// Helper
#define UBIR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UBIR,                                  \
        bf_name,                               \
        UBIR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UBIR,                                  \
        bf_name,                               \
        UBIR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

#define INC_SHIFT 0
#define INC_MASK (0xFFFF << INC_SHIFT)
UBIR_DECLARE_BIT_FIELD_FNS(INC, uint16_t);
