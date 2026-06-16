#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.3 - 7367

#define UART_UCR1_OFFSET 0x80UL

#define UCR1_VALUE_STRUCT_NAME UartUcr1Value

MMIO_DECLARE_REG32_VALUE_STRUCT(UCR1_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UCR1, UCR1_VALUE_STRUCT_NAME, UART_UCR1_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UCR1, UCR1_VALUE_STRUCT_NAME, UART_UCR1_OFFSET);

// Helper
#define UCR1_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UCR1,                                  \
        bf_name,                               \
        UCR1_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UCR1,                                  \
        bf_name,                               \
        UCR1_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

// UARTEN
#define UARTEN_SHIFT 0
#define UARTEN_MASK  (0b1 << UARTEN_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(UARTEN, bool);

// DOZE
#define DOZE_SHIFT 1
#define DOZE_MASK  (0b1 << DOZE_SHIFT)

UCR1_DECLARE_BIT_FIELD_FNS(DOZE, bool);

// ATDMAEN
#define ATDMAEN_SHIFT 2
#define ATDMAEN_MASK  (0b1 << ATDMAEN_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(ATDMAEN, bool);

// TXDAMAEN
#define TXDMAEN_SHIFT 3
#define TXDMAEN_MASK  (0b1 << TXDMAEN_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(TXDMAEN, bool);

// SNDBRK
#define SNDBRK_SHIFT 4
#define SNDBRK_MASK  (0b1 << SNDBRK_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(SNDBRK, bool);

// RTSDEN
#define RTSDEN_SHIFT 5
#define RTSDEN_MASK  (0b1 << RTSDEN_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(RTSDEN, bool);

// TXMPTYEN
#define TXMPTYEN_SHIFT 6
#define TXMPTYEN_MASK  (0b1 << TXMPTYEN_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(TXMPTYEN, bool);

// IREN
#define IREN_SHIFT 7
#define IREN_MASK  (0b1 << IREN_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(IREN, bool);

// RXDMAEN
#define RXDMAEN_SHIFT 8
#define RXDMAEN_MASK  (0b1 << RXDMAEN_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(RXDMAEN, bool);

// RRDYEN
#define RRDYEN_SHIFT 9
#define RRDYEN_MASK  (0b1 << RRDYEN_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(RRDYEN, bool);

// ICD
#define ICD_SHIFT 10
#define ICD_MASK  (0b11 << ICD_SHIFT)

typedef enum {
    UART_UCR1_ICD_IDLE_4_FRAMES  = 0b00,
    UART_UCR1_ICD_IDLE_8_FRAMES  = 0b01,
    UART_UCR1_ICD_IDLE_16_FRAMES = 0b10,
    UART_UCR1_ICD_IDLE_32_FRAMES = 0b11,
} UART_UCR1_ICD;

UCR1_DECLARE_BIT_FIELD_FNS(ICD, UART_UCR1_ICD);

// IDEN
#define IDEN_SHIFT 12
#define IDEN_MASK  (0b1 << IDEN_SHIFT)

UCR1_DECLARE_BIT_FIELD_FNS(IDEN, bool);

// TRDYEN
#define TRDYEN_SHIFT 13
#define TRDYEN_MASK  (0b1 << TRDYEN_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(TRDYEN, bool);

// ADBR
#define ADBR_SHIFT 14
#define ADBR_MASK  (0b1 << ADBR_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(ADBR, bool);

// ADEN
#define ADEN_SHIFT 15
#define ADEN_MASK  (0b1 << ADEN_SHIFT)
UCR1_DECLARE_BIT_FIELD_FNS(ADEN, bool);
