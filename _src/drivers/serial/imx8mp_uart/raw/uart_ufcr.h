#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.7 - 7376

#define UART_UFCR_OFFSET 0x90UL

#define UFCR_VALUE_STRUCT_NAME UartUfcrValue

MMIO_DECLARE_REG32_VALUE_STRUCT(UFCR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UFCR, UFCR_VALUE_STRUCT_NAME, UART_UFCR_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UFCR, UFCR_VALUE_STRUCT_NAME, UART_UFCR_OFFSET);

// Helper
#define UFCR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UFCR,                                  \
        bf_name,                               \
        UFCR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UFCR,                                  \
        bf_name,                               \
        UFCR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

// TXTL
#define TXTL_SHIFT 10
#define TXTL_MASK  (0b111111 << TXTL_SHIFT)
UFCR_DECLARE_BIT_FIELD_FNS(TXTL, uint8_t);

// RFDIV
#define RFDIV_SHIFT 7
#define RFDIV_MASK  (0b111 << RFDIV_SHIFT)

typedef enum {
    UART_UFCR_RFDIV_DIV_BY_6 = 0b000,
    UART_UFCR_RFDIV_DIV_BY_5 = 0b001,
    UART_UFCR_RFDIV_DIV_BY_4 = 0b010,
    UART_UFCR_RFDIV_DIV_BY_3 = 0b011,
    UART_UFCR_RFDIV_DIV_BY_2 = 0b100,
    UART_UFCR_RFDIV_DIV_BY_1 = 0b101,
    UART_UFCR_RFDIV_DIV_BY_7 = 0b110,
    // 111 is reserved
} UART_UFCR_RFDIV;

UFCR_DECLARE_BIT_FIELD_FNS(RFDIV, UART_UFCR_RFDIV);

// DCEDTE
#define DCEDTE_SHIFT 6
#define DCEDTE_MASK  (0b1 << DCEDTE_SHIFT)
UFCR_DECLARE_BIT_FIELD_FNS(DCEDTE, bool);

// RXTL
#define RXTL_SHIFT 0
#define RXTL_MASK  (0b111111 << RXTL_SHIFT)
UFCR_DECLARE_BIT_FIELD_FNS(RXTL, uint8_t);
