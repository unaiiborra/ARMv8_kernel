#pragma once

#ifndef DRIVERS
#    error "This header should only be imported by a driver"
#endif

#include <lib/mmio/mmio_macros.h>
#include <stdbool.h>

#include "gicv3_macros.h"

#define GICD_TYPER_OFFSET 0x4UL

#define GICD_TYPER_VALUE_STRUCT_NAME GicdTyper

MMIO_DECLARE_REG32_VALUE_STRUCT(GICD_TYPER_VALUE_STRUCT_NAME);

MMIO_DECLARE_REG32_READER(
    GICV3,
    GICD_TYPER,
    GICD_TYPER_VALUE_STRUCT_NAME,
    GICD_TYPER_OFFSET);

/* Helper: RO bitfields only */
#define GICD_TYPER_DECLARE_BIT_FIELD_GETTER(bf_name, T) \
    GICV3_DECLARE_BIT_FIELD_GETTER(                     \
        GICD_TYPER,                                     \
        bf_name,                                        \
        GICD_TYPER_VALUE_STRUCT_NAME,                   \
        T,                                              \
        bf_name##_SHIFT,                                \
        bf_name##_MASK)

/* ================= Bits [31:27] ================= */
/* ESPI_range */
#define ESPI_range_SHIFT 27
#define ESPI_range_MASK  (0x1FUL << ESPI_range_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(ESPI_range, uint8_t);

/* ================= Bit [26] ================= */
/* RSS */
#define RSS_SHIFT 26
#define RSS_MASK  (1UL << RSS_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(RSS, bool);

/* ================= Bit [25] ================= */
/* No1N */
#define No1N_SHIFT 25
#define No1N_MASK  (1UL << No1N_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(No1N, bool);

/* ================= Bit [24] ================= */
/* A3V */
#define A3V_SHIFT 24
#define A3V_MASK  (1UL << A3V_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(A3V, bool);

/* ================= Bits [23:19] ================= */
/* IDbits */
#define IDbits_SHIFT 19
#define IDbits_MASK  (0x1FUL << IDbits_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(IDbits, uint8_t);

/* ================= Bit [18] ================= */
/* DVIS */
#define DVIS_SHIFT 18
#define DVIS_MASK  (1UL << DVIS_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(DVIS, bool);

/* ================= Bit [17] ================= */
/* LPIS */
#define LPIS_SHIFT 17
#define LPIS_MASK  (1UL << LPIS_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(LPIS, bool);

/* ================= Bit [16] ================= */
/* MBIS */
#define MBIS_SHIFT 16
#define MBIS_MASK  (1UL << MBIS_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(MBIS, bool);

/* ================= Bits [15:11] ================= */
/* num_LPIs */
#define num_LPIs_SHIFT 11
#define num_LPIs_MASK  (0x1FUL << num_LPIs_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(num_LPIs, uint8_t);

/* ================= Bit [10] ================= */
/* SecurityExtn */
#define SecurityExtn_SHIFT 10
#define SecurityExtn_MASK  (1UL << SecurityExtn_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(SecurityExtn, bool);

/* ================= Bit [9] ================= */
/* NMI */
#define NMI_SHIFT 9
#define NMI_MASK  (1UL << NMI_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(NMI, bool);

/* ================= Bit [8] ================= */
/* ESPI */
#define ESPI_SHIFT 8
#define ESPI_MASK  (1UL << ESPI_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(ESPI, bool);

/* ================= Bits [7:5] ================= */
/* CPUNumber */
#define CPUNumber_SHIFT 5
#define CPUNumber_MASK  (0x7UL << CPUNumber_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(CPUNumber, uint8_t);

/* ================= Bits [4:0] ================= */
/* ITLinesNumber */
#define ITLinesNumber_SHIFT 0
#define ITLinesNumber_MASK  (0x1FUL << ITLinesNumber_SHIFT)
GICD_TYPER_DECLARE_BIT_FIELD_GETTER(ITLinesNumber, uint8_t);
