#include <arm/exceptions/exceptions.h>

.section .text
.global _el1_init_vector
.align 4
_el1_init_vector:
    adr     x0, _el1_vector_table
    msr     VBAR_EL1, x0
    isb
    ret


.macro SAVE_CONTEXT_EL1
    sub sp, sp, #ARM_STRUCT_ECTX_SIZE
    
    stp x0,  x1,  [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 0)]
    stp x2,  x3,  [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 1)]
    stp x4,  x5,  [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 2)]
    stp x6,  x7,  [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 3)]
    stp x8,  x9,  [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 4)]
    stp x10, x11, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 5)]
    stp x12, x13, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 6)]
    stp x14, x15, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 7)]
    stp x16, x17, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 8)]
    stp x18, x19, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 9)]
    stp x20, x21, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 10)]
    stp x22, x23, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 11)]
    stp x24, x25, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 12)]
    stp x26, x27, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 13)]
    stp x28, x29, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 14)]
    str x30,      [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 15)]

    mrs x0, FPCR
    mrs x1, FPSR
    stp x0, x1, [sp]

    add x9, sp, #ARM_STRUCT_ECTX_OFFSETOF_V

    st1 {v0.16b,  v1.16b},  [x9], #32
    st1 {v2.16b,  v3.16b},  [x9], #32
    st1 {v4.16b,  v5.16b},  [x9], #32
    st1 {v6.16b,  v7.16b},  [x9], #32
    st1 {v8.16b,  v9.16b},  [x9], #32
    st1 {v10.16b, v11.16b}, [x9], #32
    st1 {v12.16b, v13.16b}, [x9], #32
    st1 {v14.16b, v15.16b}, [x9], #32
    st1 {v16.16b, v17.16b}, [x9], #32
    st1 {v18.16b, v19.16b}, [x9], #32
    st1 {v20.16b, v21.16b}, [x9], #32
    st1 {v22.16b, v23.16b}, [x9], #32
    st1 {v24.16b, v25.16b}, [x9], #32
    st1 {v26.16b, v27.16b}, [x9], #32
    st1 {v28.16b, v29.16b}, [x9], #32
    st1 {v30.16b, v31.16b}, [x9]

    // load the ptr to the struct as first arg
    mov x0, sp
.endm

.macro RESTORE_CONTEXT_EL1
    ldp x0, x1, [sp]
    msr FPCR, x0
    msr FPSR, x1

    add x9, sp, #ARM_STRUCT_ECTX_OFFSETOF_V

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

    ldp x0,  x1,  [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 0)]
    ldp x2,  x3,  [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 1)]
    ldp x4,  x5,  [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 2)]
    ldp x6,  x7,  [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 3)]
    ldp x8,  x9,  [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 4)]
    ldp x10, x11, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 5)]
    ldp x12, x13, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 6)]
    ldp x14, x15, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 7)]
    ldp x16, x17, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 8)]
    ldp x18, x19, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 9)]
    ldp x20, x21, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 10)]
    ldp x22, x23, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 11)]
    ldp x24, x25, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 12)]
    ldp x26, x27, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 13)]
    ldp x28, x29, [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 14)]
    ldr x30,      [sp, #(ARM_STRUCT_ECTX_OFFSETOF_X + 16 * 15)]

    add sp, sp, #ARM_STRUCT_ECTX_SIZE
.endm

.macro EL1_CALL_WITH_CONTEXT label
    SAVE_CONTEXT_EL1
    bl \label
    RESTORE_CONTEXT_EL1
    eret
.endm

.section .text
.align 11
_el1_cur_sp0_sync:
    EL1_CALL_WITH_CONTEXT el1_cur_sp0_sync_handler

_el1_cur_sp0_irq:
    EL1_CALL_WITH_CONTEXT el1_cur_sp0_irq_handler

_el1_cur_sp0_fiq:
    EL1_CALL_WITH_CONTEXT el1_cur_sp0_fiq_handler

_el1_cur_sp0_serror:
    EL1_CALL_WITH_CONTEXT el1_cur_sp0_serror_handler

_el1_cur_spx_sync:
    EL1_CALL_WITH_CONTEXT el1_cur_spx_sync_handler

_el1_cur_spx_irq:
    EL1_CALL_WITH_CONTEXT el1_cur_spx_irq_handler

_el1_cur_spx_fiq:
    EL1_CALL_WITH_CONTEXT el1_cur_spx_fiq_handler

_el1_cur_spx_serror:
    EL1_CALL_WITH_CONTEXT el1_cur_spx_serror_handler

_el1_low_a64_sync:
    EL1_CALL_WITH_CONTEXT el1_low_a64_sync_handler

_el1_low_a64_irq:
    EL1_CALL_WITH_CONTEXT el1_low_a64_irq_handler

_el1_low_a64_fiq:
    EL1_CALL_WITH_CONTEXT el1_low_a64_fiq_handler

_el1_low_a64_serror:
    EL1_CALL_WITH_CONTEXT el1_low_a64_serror_handler

_el1_low_a32_sync:
    EL1_CALL_WITH_CONTEXT el1_low_a32_sync_handler

_el1_low_a32_irq:
    EL1_CALL_WITH_CONTEXT el1_low_a32_irq_handler

_el1_low_a32_fiq:
    EL1_CALL_WITH_CONTEXT el1_low_a32_fiq_handler

_el1_low_a32_serror:
    EL1_CALL_WITH_CONTEXT el1_low_a32_serror_handler


/* ------------------------------------------------ */
/*                  Vector table                    */
/* ------------------------------------------------ */

.macro SET_VENTRY label
.align 7
    b \label
.endm

.section .vectors, "ax"
.global _el1_vector_table
.align 11
_el1_vector_table:
//  0..3: Current EL, SP0
    SET_VENTRY _el1_cur_sp0_sync
    SET_VENTRY _el1_cur_sp0_irq
    SET_VENTRY _el1_cur_sp0_fiq
    SET_VENTRY _el1_cur_sp0_serror
//  4..7: Current EL, SPx 
    SET_VENTRY _el1_cur_spx_sync
    SET_VENTRY _el1_cur_spx_irq
    SET_VENTRY _el1_cur_spx_fiq
    SET_VENTRY _el1_cur_spx_serror
//  8..11: Lower EL, AArch64
    SET_VENTRY _el1_low_a64_sync
    SET_VENTRY _el1_low_a64_irq
    SET_VENTRY _el1_low_a64_fiq
    SET_VENTRY _el1_low_a64_serror
//  12..15: Lower EL, AArch32 
    SET_VENTRY _el1_low_a32_sync
    SET_VENTRY _el1_low_a32_irq
    SET_VENTRY _el1_low_a32_fiq
    SET_VENTRY _el1_low_a32_serror
