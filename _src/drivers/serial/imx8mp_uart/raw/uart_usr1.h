#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "uart_macros.h"

// 17.2.14.8 - 7378

#define UART_USR1_OFFSET 0x94UL

#define USR1_VALUE_STRUCT_NAME UartUsr1Value

MMIO_DECLARE_REG32_VALUE_STRUCT(USR1_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(UART, USR1, USR1_VALUE_STRUCT_NAME, UART_USR1_OFFSET);

MMIO_DECLARE_REG32_WRITER(UART, USR1, USR1_VALUE_STRUCT_NAME, UART_USR1_OFFSET);

// Helper
#define USR1_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    UART_DECLARE_BIT_FIELD_GETTER(             \
        USR1,                                  \
        bf_name,                               \
        USR1_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);                       \
    UART_DECLARE_BIT_FIELD_SETTER(             \
        USR1,                                  \
        bf_name,                               \
        USR1_VALUE_STRUCT_NAME,                \
        T,                                     \
        bf_name##_SHIFT,                       \
        bf_name##_MASK);

// PARITYERR
#define PARITYERR_SHIFT 15
#define PARITYERR_MASK  (0b1 << PARITYERR_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(PARITYERR, bool);

// RTSS
#define RTSS_SHIFT 14
#define RTSS_MASK  (0b1 << RTSS_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(RTSS, bool);

// TRDY
#define TRDY_SHIFT 13
#define TRDY_MASK  (0b1 << TRDY_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(TRDY, bool);

// RTSD
#define RTSD_SHIFT 12
#define RTSD_MASK  (0b1 << RTSD_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(RTSD, bool);

// ESCF
#define ESCF_SHIFT 11
#define ESCF_MASK  (0b1 << ESCF_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(ESCF, bool);

// FRAERR
#define FRAERR_SHIFT 10
#define FRAERR_MASK  (0b1 << FRAERR_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(FRAERR, bool);

// RRDY
#define RRDY_SHIFT 9
#define RRDY_MASK  (0b1 << RRDY_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(RRDY, bool);

// AGTIM
#define AGTIM_SHIFT 8
#define AGTIM_MASK  (0b1 << AGTIM_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(AGTIM, bool);

// DTRD  (unused)
#define DTRD_SHIFT 7
#define DTRD_MASK  (0b1 << DTRD_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(DTRD, bool);

// RXDS
#define RXDS_SHIFT 6
#define RXDS_MASK  (0b1 << RXDS_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(RXDS, bool);

// AIRINT
#define AIRINT_SHIFT 5
#define AIRINT_MASK  (0b1 << AIRINT_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(AIRINT, bool);

// AWAKE
#define AWAKE_SHIFT 4
#define AWAKE_MASK  (0b1 << AWAKE_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(AWAKE, bool);

// SAD
#define SAD_SHIFT 3
#define SAD_MASK  (0b1 << SAD_SHIFT)
USR1_DECLARE_BIT_FIELD_FNS(SAD, bool);
