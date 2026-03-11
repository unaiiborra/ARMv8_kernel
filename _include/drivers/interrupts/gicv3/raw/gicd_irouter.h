#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <kernel/panic.h>
#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gicv3_macros.h"

#define GICD_IROUTER_OFFSET(n) (0x6000UL + (8UL * n))

#define GICD_IROUTER_VALUE_STRUCT_NAME GicdIrouter

MMIO_DECLARE_REG64_VALUE_STRUCT(GICD_IROUTER_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG64_READER_N_OFFSET(
    GICV3,
    GICD_IROUTER,
    GICD_IROUTER_VALUE_STRUCT_NAME,
    GICD_IROUTER_OFFSET);

MMIO_DECLARE_REG64_WRITER_N_OFFSET(
    GICV3,
    GICD_IROUTER,
    GICD_IROUTER_VALUE_STRUCT_NAME,
    GICD_IROUTER_OFFSET);

// Helper
#define GICD_IROUTER_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    MMIO_DECLARE_BIT_FIELD_GETTER(                     \
        GICV3,                                         \
        GICD_IROUTER,                                  \
        bf_name,                                       \
        GICD_IROUTER_VALUE_STRUCT_NAME,                \
        T,                                             \
        bf_name##_SHIFT,                               \
        bf_name##_MASK)                                \
    MMIO_DECLARE_BIT_FIELD_SETTER(                     \
        GICV3,                                         \
        GICD_IROUTER,                                  \
        bf_name,                                       \
        GICD_IROUTER_VALUE_STRUCT_NAME,                \
        T,                                             \
        bf_name##_SHIFT,                               \
        bf_name##_MASK)

// Aff3 39:32
#define Aff3_SHIFT 32
#define Aff3_MASK (0xFFUL << Aff3_SHIFT)
GICD_IROUTER_DECLARE_BIT_FIELD_FNS(Aff3, uint8_t);

// Interrupt_Routing_Mode 31
#define Interrupt_Routing_Mode_SHIFT 31
#define Interrupt_Routing_Mode_MASK (1UL << Interrupt_Routing_Mode_SHIFT)
GICD_IROUTER_DECLARE_BIT_FIELD_FNS(Interrupt_Routing_Mode, bool);

// Aff2 23:16
#define Aff2_SHIFT 16
#define Aff2_MASK (0xFFUL << Aff2_SHIFT)
GICD_IROUTER_DECLARE_BIT_FIELD_FNS(Aff2, uint8_t);

// Aff1 15:8
#define Aff1_SHIFT 8
#define Aff1_MASK (0xFFUL << Aff1_SHIFT)
GICD_IROUTER_DECLARE_BIT_FIELD_FNS(Aff1, uint8_t);

// Aff0 7:0
#define Aff0_SHIFT 0
#define Aff0_MASK (0xFFUL << Aff0_SHIFT)
GICD_IROUTER_DECLARE_BIT_FIELD_FNS(Aff0, uint8_t);
