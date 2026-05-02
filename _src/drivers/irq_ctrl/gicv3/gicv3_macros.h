#pragma once

#define GICV3_DECLARE_BIT_FIELD_GETTER( \
    reg_name,                           \
    bf_name,                            \
    RegValueStruct,                     \
    T,                                  \
    SHIFT,                              \
    MASK)                               \
    MMIO_DECLARE_BIT_FIELD_GETTER(      \
        GICV3,                          \
        reg_name,                       \
        bf_name,                        \
        RegValueStruct,                 \
        T,                              \
        SHIFT,                          \
        MASK)

#define GICV3_DECLARE_BIT_FIELD_SETTER( \
    reg_name,                           \
    bf_name,                            \
    RegValueStruct,                     \
    T,                                  \
    SHIFT,                              \
    MASK)                               \
    MMIO_DECLARE_BIT_FIELD_SETTER(      \
        GICV3,                          \
        reg_name,                       \
        bf_name,                        \
        RegValueStruct,                 \
        T,                              \
        SHIFT,                          \
        MASK)

/*
 * --- GIC DISTRIBUTOR ---
 */
// Arm Generic Interrupt Controller Architecture Specification GIC architecture
// version 3 and version 4
//
// 12.8 533
#define GICV3_DISTRIBUTOR_OFFSET 0x0UL

/*
 *      --- GIC REDISTRIBUTOR ---
 */
// 12.10 634
// NOTE: This base and offset is for redistributor 0, then the stride should be
// applied for each processor ID. For the imx8mp the redistributor banks are in
// order for their cpuid. I checked it with GDB.
#define GICV3_REDISTRIBUTOR_OFFSET 0x80000UL

#define GICV3_REDISTRIBUTOR_STRIDE 0x20000UL
#define GICV3_REDISTRIBUTOR_N_OFFSET(base, n) \
    ((base) + (GICV3_REDISTRIBUTOR_OFFSET) + (n * GICV3_REDISTRIBUTOR_STRIDE))

#define GICV3_SGI_OFFSET 0x10000
#define GICV3_SGI_BASE(base, n) \
    (GICV3_REDISTRIBUTOR_N_OFFSET(base, n) + GICV3_SGI_OFFSET)
