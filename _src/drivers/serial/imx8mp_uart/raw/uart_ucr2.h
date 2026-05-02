#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.4 - 7369

#define UART_UCR2_OFFSET 0x84UL

#define UCR2_VALUE_STRUCT_NAME UartUcr2Value

MMIO_DECLARE_REG32_VALUE_STRUCT(UCR2_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UCR2, UCR2_VALUE_STRUCT_NAME, UART_UCR2_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UCR2, UCR2_VALUE_STRUCT_NAME, UART_UCR2_OFFSET);

// Helper
#define UCR2_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UCR2,                                  \
        bf_name,                               \
        UCR2_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UCR2,                                  \
        bf_name,                               \
        UCR2_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

// ESCI
#define ESCI_SHIFT 15
#define ESCI_MASK  (0b1 << ESCI_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(ESCI, bool);

// IRTS
#define IRTS_SHIFT 14
#define IRTS_MASK  (0b1 << IRTS_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(IRTS, bool);

// CTSC
#define CTSC_SHIFT 13
#define CTSC_MASK  (0b1 << CTSC_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(CTSC, bool);

// CTS
#define CTS_SHIFT 12
#define CTS_MASK  (0b1 << CTS_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(CTS, bool);

// ESCEN
#define ESCEN_SHIFT 11
#define ESCEN_MASK  (0b1 << ESCEN_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(ESCEN, bool);

// RTEC
#define RTEC_SHIFT 9
#define RTEC_MASK  (0b11 << RTEC_SHIFT)

typedef enum {
    UART_UCR2_RTEC_RISING_EDGE  = 0b00,
    UART_UCR2_RTEC_FALLING_EDGE = 0b01,
    UART_UCR2_RTEC_ANY_EDGE     = 0b10,
} UART_UCR2_RTEC;

UCR2_DECLARE_BIT_FIELD_FNS(RTEC, UART_UCR2_RTEC);

// PREN
#define PREN_SHIFT 8
#define PREN_MASK  (0b1 << PREN_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(PREN, bool);

// PROE
#define PROE_SHIFT 7
#define PROE_MASK  (0b1 << PROE_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(PROE, bool);

// STPB
#define STPB_SHIFT 6
#define STPB_MASK  (0b1 << STPB_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(STPB, bool);

// WS
#define WS_SHIFT 5
#define WS_MASK  (0b1 << WS_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(WS, bool);

// RTSEN
#define RTSEN_SHIFT 4
#define RTSEN_MASK  (0b1 << RTSEN_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(RTSEN, bool);

// ATEN
#define ATEN_SHIFT 3
#define ATEN_MASK  (0b1 << ATEN_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(ATEN, bool);

// TXEN
#define TXEN_SHIFT 2
#define TXEN_MASK  (0b1 << TXEN_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(TXEN, bool);

// RXEN
#define RXEN_SHIFT 1
#define RXEN_MASK  (0b1 << RXEN_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(RXEN, bool);

// SRST
#define SRST_SHIFT 0
#define SRST_MASK  (0b1 << SRST_SHIFT)
UCR2_DECLARE_BIT_FIELD_FNS(SRST, bool);
