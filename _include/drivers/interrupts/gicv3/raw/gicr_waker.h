#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "gicv3_macros.h"

#define GICR_WAKER_OFFSET 0x14UL

#define GICR_WAKER_VALUE_STRUCT_NAME GicrWaker

MMIO_DECLARE_REG32_VALUE_STRUCT(GICR_WAKER_VALUE_STRUCT_NAME);

static inline GICR_WAKER_VALUE_STRUCT_NAME
GICV3_GICR_WAKER_read(uintptr_t base, size_t n)
{
    return (GICR_WAKER_VALUE_STRUCT_NAME) {
        .val = *((reg32_ptr)(GICV3_REDISTRIBUTOR_N_OFFSET(base, n) +
                             (uintptr_t)(GICR_WAKER_OFFSET)))};
}

static inline void
GICV3_GICR_WAKER_write(uintptr_t base, size_t n, GICR_WAKER_VALUE_STRUCT_NAME v)
{
    *((reg32_ptr)(GICV3_REDISTRIBUTOR_N_OFFSET(base, n) +
                  (uintptr_t)(GICR_WAKER_OFFSET))) = v.val;
}

/* Helper */
#define GICR_WAKER_DECLARE_BIT_FIELD_GETTER(bf_name, T) \
    GICV3_DECLARE_BIT_FIELD_GETTER(                     \
        GICR_WAKER,                                     \
        bf_name,                                        \
        GICR_WAKER_VALUE_STRUCT_NAME,                   \
        T,                                              \
        bf_name##_SHIFT,                                \
        bf_name##_MASK)                                 \
    GICV3_DECLARE_BIT_FIELD_SETTER(                     \
        GICR_WAKER,                                     \
        bf_name,                                        \
        GICR_WAKER_VALUE_STRUCT_NAME,                   \
        T,                                              \
        bf_name##_SHIFT,                                \
        bf_name##_MASK)

// IMPLEMENTTATION DEFINED 31
#define GICR_WAKER_BIT31_SHIFT 31
#define GICR_WAKER_BIT31_MASK (1UL << GICR_WAKER_BIT31_SHIFT)
GICR_WAKER_DECLARE_BIT_FIELD_GETTER(GICR_WAKER_BIT31, bool);

// ChildrenAsleep 2
#define ChildrenAsleep_SHIFT 2
#define ChildrenAsleep_MASK (1UL << ChildrenAsleep_SHIFT)
GICR_WAKER_DECLARE_BIT_FIELD_GETTER(ChildrenAsleep, bool);

// ProcessorSleep
#define ProcessorSleep_SHIFT 1
#define ProcessorSleep_MASK (1UL << ProcessorSleep_SHIFT)
GICR_WAKER_DECLARE_BIT_FIELD_GETTER(ProcessorSleep, bool);

// IMPLEMENTTATION DEFINED 0
#define GICR_WAKER_BIT0_SHIFT 0
#define GICR_WAKER_BIT0_MASK (1UL << GICR_WAKER_BIT31_SHIFT)
GICR_WAKER_DECLARE_BIT_FIELD_GETTER(GICR_WAKER_BIT0, bool);
