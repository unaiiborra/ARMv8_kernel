#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.16 - 7387

#define UART_UTS_OFFSET 0xB4UL

#define UTS_VALUE_STRUCT_NAME UartUtsValue

MMIO_DECLARE_REG32_VALUE_STRUCT(UTS_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UTS, UTS_VALUE_STRUCT_NAME, UART_UTS_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UTS, UTS_VALUE_STRUCT_NAME, UART_UTS_OFFSET);

// Helper
#define UTS_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(            \
        UTS,                                  \
        bf_name,                              \
        UTS_VALUE_STRUCT_NAME,                \
        T,                                    \
        bf_name##_SHIFT,                      \
        bf_name##_MASK);                      \
    UART_DECLARE_BIT_FIELD_SETTER(            \
        UTS,                                  \
        bf_name,                              \
        UTS_VALUE_STRUCT_NAME,                \
        T,                                    \
        bf_name##_SHIFT,                      \
        bf_name##_MASK);

// FRCPERR
#define FRCPERR_SHIFT 13
#define FRCPERR_MASK  (0b1 << FRCPERR_SHIFT)
UTS_DECLARE_BIT_FIELD_FNS(FRCPERR, bool);

// LOOP
#define LOOP_SHIFT 12
#define LOOP_MASK  (0b1 << LOOP_SHIFT)
UTS_DECLARE_BIT_FIELD_FNS(LOOP, bool);

// DBGEN (unused)
#define DBGEN_SHIFT 11
#define DBGEN_MASK  (0b1 << DBGEN_SHIFT)
UTS_DECLARE_BIT_FIELD_FNS(DBGEN, bool);

// LOOPIR
#define LOOPIR_SHIFT 10
#define LOOPIR_MASK  (0b1 << LOOPIR_SHIFT)
UTS_DECLARE_BIT_FIELD_FNS(LOOPIR, bool);

// RXDBG (unused)
#define RXDBG_SHIFT 9
#define RXDBG_MASK  (0b1 << RXDBG_SHIFT)
UTS_DECLARE_BIT_FIELD_FNS(RXDBG, bool);

// TXEMPTY
#define TXEMPTY_SHIFT 6
#define TXEMPTY_MASK  (0b1 << TXEMPTY_SHIFT)
UTS_DECLARE_BIT_FIELD_FNS(TXEMPTY, bool);

// RXEMPTY
#define RXEMPTY_SHIFT 5
#define RXEMPTY_MASK  (0b1 << RXEMPTY_SHIFT)
UTS_DECLARE_BIT_FIELD_FNS(RXEMPTY, bool);

// TXFULL
#define TXFULL_SHIFT 4
#define TXFULL_MASK  (0b1 << TXFULL_SHIFT)
UTS_DECLARE_BIT_FIELD_FNS(TXFULL, bool);

// RXFULL
#define RXFULL_SHIFT 3
#define RXFULL_MASK  (0b1 << RXFULL_SHIFT)
UTS_DECLARE_BIT_FIELD_FNS(RXFULL, bool);

// SOFTRST
#define SOFTRST_SHIFT 0
#define SOFTRST_MASK  (0b1 << SOFTRST_SHIFT)
UTS_DECLARE_BIT_FIELD_FNS(SOFTRST, bool);
