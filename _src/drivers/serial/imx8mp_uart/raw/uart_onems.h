#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.15 - 7386

#define UART_ONEMS_OFFSET 0xB0UL

#define ONEMS_VALUE_STRUCT_NAME UartOnemsValue

MMIO_DECLARE_REG32_VALUE_STRUCT(ONEMS_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(
    UART,
    ONEMS,
    ONEMS_VALUE_STRUCT_NAME,
    UART_ONEMS_OFFSET);

MMIO_DECLARE_REG32_WRITER(
    UART,
    ONEMS,
    ONEMS_VALUE_STRUCT_NAME,
    UART_ONEMS_OFFSET);

// Helper
#define ONEMS_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(              \
        ONEMS,                                  \
        bf_name,                                \
        ONEMS_VALUE_STRUCT_NAME,                \
        T,                                      \
        bf_name##_SHIFT,                        \
        bf_name##_MASK);                        \
    UART_DECLARE_BIT_FIELD_SETTER(              \
        ONEMS,                                  \
        bf_name,                                \
        ONEMS_VALUE_STRUCT_NAME,                \
        T,                                      \
        bf_name##_SHIFT,                        \
        bf_name##_MASK);

#define ONEMS_SHIFT 0
#define ONEMS_MASK  (0xFFFFFF << ONEMS_SHIFT)
ONEMS_DECLARE_BIT_FIELD_FNS(ONEMS, bool);
