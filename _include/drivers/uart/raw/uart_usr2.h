#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.9 - 7381

#define UART_USR2_OFFSET 0x98UL

#define USR2_VALUE_STRUCT_NAME UartUsr2Value

MMIO_DECLARE_REG32_VALUE_STRUCT(USR2_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, USR2, USR2_VALUE_STRUCT_NAME, UART_USR2_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, USR2, USR2_VALUE_STRUCT_NAME, UART_USR2_OFFSET);

// Helper
#define USR2_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        USR2,                                  \
        bf_name,                               \
        USR2_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        USR2,                                  \
        bf_name,                               \
        USR2_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

// ADET
#define ADET_SHIFT 15
#define ADET_MASK (0b1 << ADET_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(ADET, bool);

// TXFE
#define TXFE_SHIFT 14
#define TXFE_MASK (0b1 << TXFE_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(TXFE, bool);

// DTRF (unused)
#define DTRF_SHIFT 13
#define DTRF_MASK (0b1 << DTRF_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(DTRF, bool);

// IDLE
#define IDLE_SHIFT 12
#define IDLE_MASK (0b1 << IDLE_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(IDLE, bool);

// ACST
#define ACST_SHIFT 11
#define ACST_MASK (0b1 << ACST_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(ACST, bool);

// RIDELT (unused)
#define RIDELT_SHIFT 10
#define RIDELT_MASK (0b1 << RIDELT_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(RIDELT, bool);

// RIIN (unused)
#define RIIN_SHIFT 9
#define RIIN_MASK (0b1 << RIIN_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(RIIN, bool);

// IRINT
#define IRINT_SHIFT 8
#define IRINT_MASK (0b1 << IRINT_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(IRINT, bool);

// WAKE
#define WAKE_SHIFT 7
#define WAKE_MASK (0b1 << WAKE_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(WAKE, bool);

// DCDDELT (unused)
#define DCDDELT_SHIFT 6
#define DCDDELT_MASK (0b1 << DCDDELT_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(DCDDELT, bool);

// DCDIN (unused)
#define DCDIN_SHIFT 5
#define DCDIN_MASK (0b1 << DCDIN_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(DCDIN, bool);

// RTSF
#define RTSF_SHIFT 4
#define RTSF_MASK (0b1 << RTSF_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(RTSF, bool);

// TXDC
#define TXDC_SHIFT 3
#define TXDC_MASK (0b1 << TXDC_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(TXDC, bool);

// BRCD
#define BRCD_SHIFT 2
#define BRCD_MASK (0b1 << BRCD_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(BRCD, bool);

// ORE
#define ORE_SHIFT 1
#define ORE_MASK (0b1 << ORE_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(ORE, bool);

// RDR
#define RDR_SHIFT 0
#define RDR_MASK (0b1 << RDR_SHIFT)
USR2_DECLARE_BIT_FIELD_FNS(RDR, bool);
