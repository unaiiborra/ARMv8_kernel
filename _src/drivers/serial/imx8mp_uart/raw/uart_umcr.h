#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.17 - 7388

#define UART_UMCR_OFFSET 0xB8UL

#define UMCR_VALUE_STRUCT_NAME UartUmcrValue

MMIO_DECLARE_REG32_VALUE_STRUCT(UMCR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UMCR, UMCR_VALUE_STRUCT_NAME, UART_UMCR_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UMCR, UMCR_VALUE_STRUCT_NAME, UART_UMCR_OFFSET);

// Helper
#define UMCR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UMCR,                                  \
        bf_name,                               \
        UMCR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UMCR,                                  \
        bf_name,                               \
        UMCR_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

// SLADDR
#define SLADDR_SHIFT 8
#define SLADDR_MASK  (0xFF << SLADDR_SHIFT)
UMCR_DECLARE_BIT_FIELD_FNS(SLADDR, uint8_t);

// SADEN
#define SADEN_SHIFT 3
#define SADEN_MASK  (0b1 << SADEN_SHIFT)
UMCR_DECLARE_BIT_FIELD_FNS(SADEN, bool);

// TXB8
#define TXB8_SHIFT 2
#define TXB8_MASK  (0b1 << TXB8_SHIFT)
UMCR_DECLARE_BIT_FIELD_FNS(TXB8, bool);

// SLAM
#define SLAM_SHIFT 1
#define SLAM_MASK  (0b1 << SLAM_SHIFT)
UMCR_DECLARE_BIT_FIELD_FNS(SLAM, bool);

// MDEN
#define MDEN_SHIFT 0
#define MDEN_MASK  (0b1 << MDEN_SHIFT)
UMCR_DECLARE_BIT_FIELD_FNS(MDEN, bool);
