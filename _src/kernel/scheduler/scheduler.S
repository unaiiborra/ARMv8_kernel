#include <arm/exceptions/exceptions.h>

.section .text

/*
extern void _scheduler_loop_cpu_enter(
    x0: arm_exception_ctx* el0_ctx,
    x1: uint64_t sp_el0,
    x2: uint64_t pc
    x3: uint64_t el1_ctx[2],
    );
*/
.global _scheduler_loop_cpu_enter
_scheduler_loop_cpu_enter:    
    msr SP_EL0, x1
    msr ELR_EL1, x2

    // save fp and lr and stack in el1 ctx just in case a
    // future exit from the scheduler mode is needed
    stp x29, x30, [x3]
    mov x4, sp
    stp  x4, xzr, [x3, #16]

    // load el0 ctx
    ldp x2, x3, [x0]
    
    mrs x2, FPCR
    mrs x3, FPSR

    add x9, x0, #ARM_STRUCT_ECTX_OFFSETOF_V

    ld1 {v0.16b,  v1.16b},  [x9], #32
    ld1 {v2.16b,  v3.16b},  [x9], #32
    ld1 {v4.16b,  v5.16b},  [x9], #32
    ld1 {v6.16b,  v7.16b},  [x9], #32
    ld1 {v8.16b,  v9.16b},  [x9], #32
    ld1 {v10.16b, v11.16b}, [x9], #32
    ld1 {v12.16b, v13.16b}, [x9], #32
    ld1 {v14.16b, v15.16b}, [x9], #32
    ld1 {v16.16b, v17.16b}, [x9], #32
    ld1 {v18.16b, v19.16b}, [x9], #32
    ld1 {v20.16b, v21.16b}, [x9], #32
    ld1 {v22.16b, v23.16b}, [x9], #32
    ld1 {v24.16b, v25.16b}, [x9], #32
    ld1 {v26.16b, v27.16b}, [x9], #32
    ld1 {v28.16b, v29.16b}, [x9], #32
    ld1 {v30.16b, v31.16b}, [x9], #32    

    ldp x30, xzr, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 15)]
    ldp x28, x29, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 14)]
    ldp x26, x27, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 13)]
    ldp x24, x25, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 12)]
    ldp x22, x23, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 11)]
    ldp x20, x21, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 10)]
    ldp x18, x19, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 9)]
    ldp x16, x17, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 8)]
    ldp x14, x15, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 7)]
    ldp x12, x13, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 6)]
    ldp x10, x11, [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 5)]
    ldp x8,  x9,  [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 4)]
    ldp x6,  x7,  [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 3)]
    ldp x4,  x5,  [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 2)]
    ldp x2,  x3,  [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 1)]
    ldp x0,  x1,  [x0, #(ARM_STRUCT_ECTX_OFFSETOF_X)]

    // eret to el0
    msr SPSR_EL1, xzr
    eret

    

// noreturn void _scheduler_loop_cpu_exit(uint64_t el1_ctx[2]);
.global _scheduler_loop_cpu_exit
_scheduler_loop_cpu_exit:
    ldp x29, x30, [x0]
    ldr x1,       [x0, #16]
    mov sp, x1

    stp xzr, xzr, [x0]
    stp xzr, xzr, [x0, #16]

    msr ELR_EL1, x30
    msr SP_EL0, xzr


    // return to el1 (out of exception)
    mov x0, #0x5 // 0b0101 EL1 with SP_EL1 (EL1h).
    msr SPSR_EL1, x0
    eret
    
