#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.5 - 7372

#define UART_UCR3_OFFSET 0x88UL

#define UCR3_VALUE_STRUCT_NAME UartUcr3Value

MMIO_DECLARE_REG32_VALUE_STRUCT(UCR3_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, UCR3, UCR3_VALUE_STRUCT_NAME, UART_UCR3_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, UCR3, UCR3_VALUE_STRUCT_NAME, UART_UCR3_OFFSET);

// Helper
#define UCR3_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        UCR3,                                  \
        bf_name,                               \
        UCR3_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        UCR3,                                  \
        bf_name,                               \
        UCR3_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

// DPEC (unused)
#define DPEC_SHIFT 14
#define DPEC_MASK  (0b11 << DPEC_SHIFT)
typedef enum {
    UART_UCR3_DPEC_UNUSED = 0b00, // All values unused on this chip
} UART_UCR3_DPEC;
UCR3_DECLARE_BIT_FIELD_FNS(DPEC, UART_UCR3_DPEC);

// DTREN (unused)
#define DTREN_SHIFT 13
#define DTREN_MASK  (0b1 << DTREN_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(DTREN, bool);

// PARERREN
#define PARERREN_SHIFT 12
#define PARERREN_MASK  (0b1 << PARERREN_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(PARERREN, bool);

// FRAERREN
#define FRAERREN_SHIFT 11
#define FRAERREN_MASK  (0b1 << FRAERREN_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(FRAERREN, bool);

// DSR (unused)
#define DSR_SHIFT 10
#define DSR_MASK  (0b1 << DSR_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(DSR, bool);

// DCD (unused)
#define DCD_SHIFT 9
#define DCD_MASK  (0b1 << DCD_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(DCD, bool);

// RI (unused)
#define RI_SHIFT 8
#define RI_MASK  (0b1 << RI_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(RI, bool);

// ADNIMP
#define ADNIMP_SHIFT 7
#define ADNIMP_MASK  (0b1 << ADNIMP_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(ADNIMP, bool);

// RXDSEN
#define RXDSEN_SHIFT 6
#define RXDSEN_MASK  (0b1 << RXDSEN_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(RXDSEN, bool);

// AIRINTEN
#define AIRINTEN_SHIFT 5
#define AIRINTEN_MASK  (0b1 << AIRINTEN_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(AIRINTEN, bool);

// AWAKEN
#define AWAKEN_SHIFT 4
#define AWAKEN_MASK  (0b1 << AWAKEN_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(AWAKEN, bool);

// DTRDEN (unused)
#define DTRDEN_SHIFT 3
#define DTRDEN_MASK  (0b1 << DTRDEN_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(DTRDEN, bool);

// RXDMUXSEL
#define RXDMUXSEL_SHIFT 2
#define RXDMUXSEL_MASK  (0b1 << RXDMUXSEL_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(RXDMUXSEL, bool);

// INVT
#define INVT_SHIFT 1
#define INVT_MASK  (0b1 << INVT_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(INVT, bool);

// ACIEN
#define ACIEN_SHIFT 0
#define ACIEN_MASK  (0b1 << ACIEN_SHIFT)
UCR3_DECLARE_BIT_FIELD_FNS(ACIEN, bool);
