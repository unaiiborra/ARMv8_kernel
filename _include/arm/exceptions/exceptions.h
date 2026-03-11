#pragma once

#ifndef __ASSEMBLER__
#    include <stdbool.h>
#    include <stddef.h>
#    include <stdint.h>
typedef struct {
    bool fiq;
    bool irq;
    bool serror;
    bool debug;
} arm_exception_status;

/// true means enabled
arm_exception_status arm_exceptions_get_status();

/// true means enable, false disable
void arm_exceptions_set_status(arm_exception_status status);

/// Enables exceptions on true. If a param is false, the current state of the
/// exception will be mantained, not disabled.
void arm_exceptions_enable(bool fiq, bool irq, bool serror, bool debug);

/// Disables exceptions on true. If a param is false, the current state of the
/// exception will be mantained, not enabled.
void arm_exceptions_disable(bool fiq, bool irq, bool serror, bool debug);

size_t arm_get_exception_level();


typedef struct {
    uint64_t fpcr;
    uint64_t fpsr;
    uint64_t x[31];                 // x0-x30
    _Alignas(16) uint64_t v[32][2]; // v0-v31
} arm_exception_ctx;
#endif


#define ARM_STRUCT_ECTX_SIZE 784
#define ARM_STRUCT_ECTX_OFFSETOF_X 16
#define ARM_STRUCT_ECTX_OFFSETOF_V 272


#ifndef __ASSEMBLER__
_Static_assert(ARM_STRUCT_ECTX_SIZE == sizeof(arm_exception_ctx));
_Static_assert(
    __builtin_offsetof(arm_exception_ctx, x) == ARM_STRUCT_ECTX_OFFSETOF_X);
_Static_assert(
    __builtin_offsetof(arm_exception_ctx, v) == ARM_STRUCT_ECTX_OFFSETOF_V);
#endif
