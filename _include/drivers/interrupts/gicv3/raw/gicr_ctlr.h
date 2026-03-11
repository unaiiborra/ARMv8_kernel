#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gicv3_macros.h"

#define GICR_CTLR_OFFSET 0x0UL

#define GICR_CTLR_VALUE_STRUCT_NAME GicrCtlr

MMIO_DECLARE_REG32_VALUE_STRUCT(GICR_CTLR_VALUE_STRUCT_NAME);

static inline GICR_CTLR_VALUE_STRUCT_NAME
GICV3_GICR_CTLR_read(uintptr_t base, size_t n)
{
    return (GICR_CTLR_VALUE_STRUCT_NAME) {
        .val = *((reg32_ptr)(GICV3_REDISTRIBUTOR_N_OFFSET(base, n) +
                             (GICR_CTLR_OFFSET)))};
}

static inline void
GICV3_GICR_CTLR_write(uintptr_t base, size_t n, GICR_CTLR_VALUE_STRUCT_NAME v)
{
    *((reg32_ptr)(GICV3_REDISTRIBUTOR_N_OFFSET(base, n) + (GICR_CTLR_OFFSET))) =
        v.val;
}

/* Helpers */
#define GICR_CTLR_DECLARE_BIT_FIELD_FNS(bf_name, T) \
    GICV3_DECLARE_BIT_FIELD_GETTER(                 \
        GICR_CTLR,                                  \
        bf_name,                                    \
        GICR_CTLR_VALUE_STRUCT_NAME,                \
        T,                                          \
        bf_name##_SHIFT,                            \
        bf_name##_MASK)                             \
    GICV3_DECLARE_BIT_FIELD_SETTER(                 \
        GICR_CTLR,                                  \
        bf_name,                                    \
        GICR_CTLR_VALUE_STRUCT_NAME,                \
        T,                                          \
        bf_name##_SHIFT,                            \
        bf_name##_MASK)

/* ---------------- UWP [31] (RO) ---------------- */
#define UWP_SHIFT 31
#define UWP_MASK (1UL << UWP_SHIFT)
GICR_CTLR_DECLARE_BIT_FIELD_FNS(UWP, bool);

/* ---------------- DPG1S [26] ---------------- */
#define DPG1S_SHIFT 26
#define DPG1S_MASK (1UL << DPG1S_SHIFT)
GICR_CTLR_DECLARE_BIT_FIELD_FNS(DPG1S, bool);

/* ---------------- DPG1NS [25] ---------------- */
#define DPG1NS_SHIFT 25
#define DPG1NS_MASK (1UL << DPG1NS_SHIFT)
GICR_CTLR_DECLARE_BIT_FIELD_FNS(DPG1NS, bool);

/* ---------------- DPG0 [24] ---------------- */
#define DPG0_SHIFT 24
#define DPG0_MASK (1UL << DPG0_SHIFT)
GICR_CTLR_DECLARE_BIT_FIELD_FNS(DPG0, bool);

/* ---------------- RWP [3] (RO) ---------------- */
#define RWP_SHIFT 3
#define RWP_MASK (1UL << RWP_SHIFT)
GICR_CTLR_DECLARE_BIT_FIELD_FNS(RWP, bool);

/* ---------------- IR [2] (RO) ---------------- */
#define IR_SHIFT 2
#define IR_MASK (1UL << IR_SHIFT)
GICR_CTLR_DECLARE_BIT_FIELD_FNS(IR, bool);

/* ---------------- CES [1] (RO) ---------------- */
#define CES_SHIFT 1
#define CES_MASK (1UL << CES_SHIFT)
GICR_CTLR_DECLARE_BIT_FIELD_FNS(CES, bool);

/* ---------------- EnableLPIs [0] ---------------- */
#define EnableLPIs_SHIFT 0
#define EnableLPIs_MASK (1UL << EnableLPIs_SHIFT)
GICR_CTLR_DECLARE_BIT_FIELD_FNS(EnableLPIs, bool);
