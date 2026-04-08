#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.6 - 7374

#define UART_UCR4_OFFSET 0x8CUL

#define UCR4_VALUE_STRUCT_NAME UartUcr4Value

MMIO_DECLARE_REG32_VALUE_STRUCT(UCR4_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UCR4, UCR4_VALUE_STRUCT_NAME, UART_UCR4_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UCR4, UCR4_VALUE_STRUCT_NAME, UART_UCR4_OFFSET);

// Helper
#define UCR4_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UCR4,                                  \
        bf_name,                               \
        UCR4_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UCR4,                                  \
        bf_name,                               \
        UCR4_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

// CTSTL
#define CTSTL_SHIFT 10
#define CTSTL_MASK  (0b111111 << CTSTL_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(CTSTL, uint8_t);

// INVR
#define INVR_SHIFT 9
#define INVR_MASK  (0b1 << INVR_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(INVR, bool);

// ENIRI
#define ENIRI_SHIFT 8
#define ENIRI_MASK  (0b1 << ENIRI_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(ENIRI, bool);

// WKEN
#define WKEN_SHIFT 7
#define WKEN_MASK  (0b1 << WKEN_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(WKEN, bool);

// IDDMAEN
#define IDDMAEN_SHIFT 6
#define IDDMAEN_MASK  (0b1 << IDDMAEN_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(IDDMAEN, bool);

// IRSC
#define IRSC_SHIFT 5
#define IRSC_MASK  (0b1 << IRSC_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(IRSC, bool);

// LPBYP
#define LPBYP_SHIFT 4
#define LPBYP_MASK  (0b1 << LPBYP_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(LPBYP, bool);

// TCEN
#define TCEN_SHIFT 3
#define TCEN_MASK  (0b1 << TCEN_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(TCEN, bool);

// BKEN
#define BKEN_SHIFT 2
#define BKEN_MASK  (0b1 << BKEN_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(BKEN, bool);

// OREN
#define OREN_SHIFT 1
#define OREN_MASK  (0b1 << OREN_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(OREN, bool);

// DREN
#define DREN_SHIFT 0
#define DREN_MASK  (0b1 << DREN_SHIFT)
UCR4_DECLARE_BIT_FIELD_FNS(DREN, bool);
