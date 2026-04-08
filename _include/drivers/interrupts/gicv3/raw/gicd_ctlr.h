#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "gicv3_macros.h"

#define GICD_CTLR_OFFSET 0x0UL

#define GICD_CTLR_VALUE_STRUCT_NAME GicdCtlr

MMIO_DECLARE_REG32_VALUE_STRUCT(GICD_CTLR_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(
    GICV3,
    GICD_CTLR,
    GICD_CTLR_VALUE_STRUCT_NAME,
    GICD_CTLR_OFFSET);

MMIO_DECLARE_REG32_WRITER(
    GICV3,
    GICD_CTLR,
    GICD_CTLR_VALUE_STRUCT_NAME,
    GICD_CTLR_OFFSET);

// Helper
#define GICD_CTLR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    GICV3_DECLARE_BIT_FIELD_GETTER(                 \
        GICD_CTLR,                                  \
        bf_name,                                    \
        GICD_CTLR_VALUE_STRUCT_NAME,                \
        T,                                          \
        bf_name##_SHIFT,                            \
        bf_name##_MASK);                            \
    GICV3_DECLARE_BIT_FIELD_SETTER(                 \
        GICD_CTLR,                                  \
        bf_name,                                    \
        GICD_CTLR_VALUE_STRUCT_NAME,                \
        T,                                          \
        bf_name##_SHIFT,                            \
        bf_name##_MASK);

// RWP
#define RWP_SHIFT 31
#define RWP_MASK  (1UL << RWP_SHIFT)
GICD_CTLR_DECLARE_BIT_FIELD_FNS(RWP, bool);

// E1NWF
#define E1NWF_SHIFT 7
#define E1NWF_MASK  (1UL << E1NWF_SHIFT)
GICD_CTLR_DECLARE_BIT_FIELD_FNS(E1NWF, bool);

// DS
#define DS_SHIFT 6
#define DS_MASK  (1UL << DS_SHIFT)
GICD_CTLR_DECLARE_BIT_FIELD_FNS(DS, bool)

// ARE_NS
#define ARE_NS_SHIFT 5
#define ARE_NS_MASK  (1UL << ARE_NS_SHIFT)
GICD_CTLR_DECLARE_BIT_FIELD_FNS(ARE_NS, bool);

// ARE_S
#define ARE_S_SHIFT 4
#define ARE_S_MASK  (1UL << ARE_S_SHIFT)
GICD_CTLR_DECLARE_BIT_FIELD_FNS(ARE_S, bool);

// EnableGrp1S
#define EnableGrp1S_SHIFT 2
#define EnableGrp1S_MASK  (1UL << EnableGrp1S_SHIFT)
GICD_CTLR_DECLARE_BIT_FIELD_FNS(EnableGrp1S, bool);

// EnableGrp1NS
#define EnableGrp1NS_SHIFT 1
#define EnableGrp1NS_MASK  (1UL << EnableGrp1NS_SHIFT)
GICD_CTLR_DECLARE_BIT_FIELD_FNS(EnableGrp1NS, bool);

// EnableGrp0
#define EnableGrp0_SHIFT 0
#define EnableGrp0_MASK  (1UL << EnableGrp0_SHIFT)
GICD_CTLR_DECLARE_BIT_FIELD_FNS(EnableGrp0, bool);
