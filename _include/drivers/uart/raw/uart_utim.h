#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.11 - 7383

#define UART_UTIM_OFFSET 0xA0UL

#define UTIM_VALUE_STRUCT_NAME UartUtimValue

MMIO_DECLARE_REG32_VALUE_STRUCT(UTIM_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UTIM, UTIM_VALUE_STRUCT_NAME, UART_UTIM_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UTIM, UTIM_VALUE_STRUCT_NAME, UART_UTIM_OFFSET);

// Helper
#define UTIM_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UTIM,                                  \
        bf_name,                               \
        UTIM_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UTIM,                                  \
        bf_name,                               \
        UTIM_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

#define TIM_SHIFT 0
#define TIM_MASK (0xFFF << TIM_SHIFT)
UTIM_DECLARE_BIT_FIELD_FNS(TIM, bool);
