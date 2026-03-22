#pragma once

#define ECTX_IMPL

#define ARM_STRUCT_ECTX_SIZE 800

#define ARM_STRUCT_ECTX_OFFS_FPCR 0
#define ARM_STRUCT_ECTX_OFFS_FPSR 8
#define ARM_STRUCT_ECTX_OFFS_ELR 16
#define ARM_STRUCT_ECTX_OFFS_SPSR 24
#define ARM_STRUCT_ECTX_OFFS_SP_ELX 32
#define ARM_STRUCT_ECTX_OFFS_X 40
#define ARM_STRUCT_ECTX_OFFS_V 288

#define ARM_STRUCT_ECTX_SIZEOF_X 8
#define ARM_STRUCT_ECTX_SIZEOF_V 16


#ifndef __ASSEMBLER__

#    include <arm/reg.h>
#    include <kernel/panic.h>
#    include <stdbool.h>
#    include <stddef.h>

/// arm exception context. Struct whose purpose is to store the state of the
/// processor when a exception happens or when restoring the previously stored
/// state. Macros for hadling the struct have been made in asm
typedef struct {
    sysreg_t fpcr;
    sysreg_t fpsr;
    sysreg_t elr;
    sysreg_t spsr;

    sp_t sp_elx;

    gpr_t x[XREG_COUNT];     // x0-x30
    vreg128_t v[VREG_COUNT]; // v0-v31
} arm_ectx;


_Static_assert(sizeof(arm_ectx) % 16 == 0, "");

_Static_assert(
    ARM_STRUCT_ECTX_SIZE == sizeof(arm_ectx),
    "arm_ectx size mismatch");

_Static_assert(
    offsetof(arm_ectx, sp_elx) == ARM_STRUCT_ECTX_OFFS_SP_ELX,
    "sp_elx offset mismatch");

_Static_assert(
    offsetof(arm_ectx, fpcr) == ARM_STRUCT_ECTX_OFFS_FPCR,
    "fpcr offset mismatch");

_Static_assert(
    offsetof(arm_ectx, fpsr) == ARM_STRUCT_ECTX_OFFS_FPSR,
    "fpsr offset mismatch");

_Static_assert(
    offsetof(arm_ectx, elr) == ARM_STRUCT_ECTX_OFFS_ELR,
    "elr offset mismatch");

_Static_assert(
    offsetof(arm_ectx, spsr) == ARM_STRUCT_ECTX_OFFS_SPSR,
    "spsr offset mismatch");

_Static_assert(
    offsetof(arm_ectx, x) == ARM_STRUCT_ECTX_OFFS_X,
    "x offset mismatch");

_Static_assert(
    offsetof(arm_ectx, v) == ARM_STRUCT_ECTX_OFFS_V,
    "v offset mismatch");

_Static_assert(
    ARM_STRUCT_ECTX_SIZEOF_X == sizeof(gpr_t),
    "arm_ectx x size mismatch");

_Static_assert(
    ARM_STRUCT_ECTX_SIZEOF_V == sizeof(vreg128_t),
    "arm_ectx v size mismatch");


typedef enum {
    // https://developer.arm.com/documentation/ddi0487/maa/-Part-C-The-AArch64-Instruction-Set/-Chapter-C5-The-A64-System-Instruction-Class/-C5-2-Special-purpose-registers/-C5-2-20-SPSR-EL1--Saved-Program-Status-Register--EL1-
    SPSR_EL0 = 0b0000ul,  // EL0.
    SPSR_EL1t = 0b0100ul, // EL1 with SP_EL0 (EL1t).
    SPSR_EL1h = 0b0101ul, // EL1 with SP_EL1 (EL1h).
} arm_spsr_m_mode;


typedef enum {
    SP_EL0 = 0,
    SP_EL1 = 1,
} arm_stack;


typedef enum {
    EL0 = 0,
    EL1 = 1,
} arm_exception_level;


static inline arm_stack ectx_stack(const arm_ectx* ectx)
{
    return (arm_stack)(ectx->spsr & 1);
}

static inline arm_exception_level ectx_el(const arm_ectx* ectx)
{
    return (arm_exception_level)((ectx->spsr & (1ul << 2)) ? EL1 : EL0);
}


static inline arm_spsr_m_mode ectx_get_aarch64_selected_el(const arm_ectx* ectx)
{
    switch (ectx->spsr & 0b1111ul) {
        case SPSR_EL0:
            return SPSR_EL0;
        case SPSR_EL1t:
            return SPSR_EL1t;
        case SPSR_EL1h:
            return SPSR_EL1h;

        case 0b1000:
            PANIC(
                "EL1 with SP_EL0 (EL1t) and either HCR_EL2.{NV, NV1} is {1,0} "
                "or HCR_EL2.{NV, NV2} is {1,1}. not supported!");
        case 0b1001:
            PANIC(
                "EL1 with SP_EL1 (EL1h) and either HCR_EL2.{NV, NV1} is {1,0} "
                "or HCR_EL2.{NV, NV2} is {1,1}. not supported!");
        default:
            PANIC("ectx_get_aarch64_selected_el: unknown value");
    }
}


#else // __ASSEMBLER__

#    include "ctx.S"

#endif