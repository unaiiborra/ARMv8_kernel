#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <arm/cpu.h>
#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gicv3_macros.h"

#define GICR_TYPER_OFFSET 0x8UL

#define GICR_TYPER_VALUE_STRUCT_NAME GicrTyper

MMIO_DECLARE_REG64_VALUE_STRUCT(GICR_TYPER_VALUE_STRUCT_NAME);

static inline GICR_TYPER_VALUE_STRUCT_NAME
GICV3_GICR_TYPER_read(uintptr_t base, size_t n)
{
    return (GICR_TYPER_VALUE_STRUCT_NAME) {
        .val = *((reg32_ptr)(GICV3_REDISTRIBUTOR_N_OFFSET(base, n) +
                             (uintptr_t)(GICR_TYPER_OFFSET)))};
}

static inline void
GICV3_GICR_TYPER_write(uintptr_t base, size_t n, GICR_TYPER_VALUE_STRUCT_NAME v)
{
    *((reg32_ptr)(GICV3_REDISTRIBUTOR_N_OFFSET(base, n) +
                  (uintptr_t)(GICR_TYPER_OFFSET))) = v.val;
}

/* Helper */
#define GICR_TYPER_DECLARE_BIT_FIELD_GETTER(bf_name, T) \
    GICV3_DECLARE_BIT_FIELD_GETTER(                     \
        GICR_TYPER,                                     \
        bf_name,                                        \
        GICR_TYPER_VALUE_STRUCT_NAME,                   \
        T,                                              \
        bf_name##_SHIFT,                                \
        bf_name##_MASK)

/* ---------------- AffinityValue [63:32] ---------------- */
#define AffinityValue_SHIFT 32
#define AffinityValue_MASK  (0xFFFFFFFFULL << AffinityValue_SHIFT)

static inline ARM_cpu_affinity
GICV3_GICR_TYPER_BF_get_AffinityValue(const GicrTyper r)
{
    uint32_t affinity =
        (uint32_t)((r.val & AffinityValue_MASK) >> AffinityValue_SHIFT);

    return CPU_AFFINITY_FROM_U32(affinity);
}

/* ---------------- PPInum [31:27] ---------------- */
#define PPInum_SHIFT 27
#define PPInum_MASK  (0x1FUL << PPInum_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(PPInum, uint32_t);

/* ---------------- VSGI [26] ---------------- */
#define VSGI_SHIFT 26
#define VSGI_MASK  (1UL << VSGI_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(VSGI, bool);

/* ---------------- CommonLPIAff [25:24] ---------------- */
#define CommonLPIAff_SHIFT 24
#define CommonLPIAff_MASK  (0x3UL << CommonLPIAff_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(CommonLPIAff, uint32_t);

/* ---------------- Processor_Number [23:8] ---------------- */
#define Processor_Number_SHIFT 8
#define Processor_Number_MASK  (0xFFFFUL << Processor_Number_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(Processor_Number, uint32_t);

/* ---------------- RVPEID [7] ---------------- */
#define RVPEID_SHIFT 7
#define RVPEID_MASK  (1UL << RVPEID_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(RVPEID, bool);

/* ---------------- MPAM [6] ---------------- */
#define MPAM_SHIFT 6
#define MPAM_MASK  (1UL << MPAM_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(MPAM, bool);

/* ---------------- DPGS [5] ---------------- */
#define DPGS_SHIFT 5
#define DPGS_MASK  (1UL << DPGS_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(DPGS, bool);

/* ---------------- Last [4] ---------------- */
#define Last_SHIFT 4
#define Last_MASK  (1UL << Last_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(Last, bool);

/* ---------------- DirectLPI [3] ---------------- */
#define DirectLPI_SHIFT 3
#define DirectLPI_MASK  (1UL << DirectLPI_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(DirectLPI, bool);

/* ---------------- Dirty [2] ---------------- */
#define Dirty_SHIFT 2
#define Dirty_MASK  (1UL << Dirty_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(Dirty, bool);

/* ---------------- VLPIS [1] ---------------- */
#define VLPIS_SHIFT 1
#define VLPIS_MASK  (1UL << VLPIS_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(VLPIS, bool);

/* ---------------- PLPIS [0] ---------------- */
#define PLPIS_SHIFT 0
#define PLPIS_MASK  (1UL << PLPIS_SHIFT)
GICR_TYPER_DECLARE_BIT_FIELD_GETTER(PLPIS, bool);
