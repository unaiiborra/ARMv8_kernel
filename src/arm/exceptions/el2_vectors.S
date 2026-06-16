.section .text
.global _el2_init_vector
.align 4
_el2_init_vector:
    adr     x0, _el2_vector_table
    msr     VBAR_EL2, x0
    isb
    ret

.set GPR_PAIRS, 16
.set GPR_FRAME_SIZE, (GPR_PAIRS) * 16

// TODO: Store v registers
.macro SAVE_CONTEXT_EL2
    sub sp, sp,   #GPR_FRAME_SIZE
    stp x0, x1,   [sp, #16 * 0]
    stp x2, x3,   [sp, #16 * 1]
    stp x4, x5,   [sp, #16 * 2]
    stp x6, x7,   [sp, #16 * 3]
    stp x8, x9,   [sp, #16 * 4]
    stp x10, x11, [sp, #16 * 5]
    stp x12, x13, [sp, #16 * 6]
    stp x14, x15, [sp, #16 * 7]
    stp x16, x17, [sp, #16 * 8]
    stp x18, x19, [sp, #16 * 9]
    stp x20, x21, [sp, #16 * 10]
    stp x22, x23, [sp, #16 * 11]
    stp x24, x25, [sp, #16 * 12]
    stp x26, x27, [sp, #16 * 13]
    stp x28, x29, [sp, #16 * 14]
    str x30,      [sp, #16 * 15]
.endm

.macro RESTORE_CONTEXT_EL2
    ldp x0, x1,   [sp, #16 * 0]
    ldp x2, x3,   [sp, #16 * 1]
    ldp x4, x5,   [sp, #16 * 2]
    ldp x6, x7,   [sp, #16 * 3]
    ldp x8, x9,   [sp, #16 * 4]
    ldp x10, x11, [sp, #16 * 5]
    ldp x12, x13, [sp, #16 * 6]
    ldp x14, x15, [sp, #16 * 7]
    ldp x16, x17, [sp, #16 * 8]
    ldp x18, x19, [sp, #16 * 9]
    ldp x20, x21, [sp, #16 * 10]
    ldp x22, x23, [sp, #16 * 11]
    ldp x24, x25, [sp, #16 * 12]
    ldp x26, x27, [sp, #16 * 13]
    ldp x28, x29, [sp, #16 * 14]
    ldr x30,      [sp, #16 * 15]
    add sp, sp,   #GPR_FRAME_SIZE
.endm

.macro EL2_CALL_WITH_CONTEXT label
    SAVE_CONTEXT_EL2
    bl \label
    RESTORE_CONTEXT_EL2
    eret
.endm

.section .text
.align 11
_el2_cur_sp0_sync:
    EL2_CALL_WITH_CONTEXT el2_cur_sp0_sync_handler

_el2_cur_sp0_irq:
    EL2_CALL_WITH_CONTEXT el2_cur_sp0_irq_handler

_el2_cur_sp0_fiq:
    EL2_CALL_WITH_CONTEXT el2_cur_sp0_fiq_handler

_el2_cur_sp0_serror:
    EL2_CALL_WITH_CONTEXT el2_cur_sp0_serror_handler

_el2_cur_spx_sync:
    EL2_CALL_WITH_CONTEXT el2_cur_spx_sync_handler

_el2_cur_spx_irq:
    EL2_CALL_WITH_CONTEXT el2_cur_spx_irq_handler

_el2_cur_spx_fiq:
    EL2_CALL_WITH_CONTEXT el2_cur_spx_fiq_handler

_el2_cur_spx_serror:
    EL2_CALL_WITH_CONTEXT el2_cur_spx_serror_handler

_el2_low_a64_sync:
    EL2_CALL_WITH_CONTEXT el2_low_a64_sync_handler

_el2_low_a64_irq:
    EL2_CALL_WITH_CONTEXT el2_low_a64_irq_handler

_el2_low_a64_fiq:
    EL2_CALL_WITH_CONTEXT el2_low_a64_fiq_handler

_el2_low_a64_serror:
    EL2_CALL_WITH_CONTEXT el2_low_a64_serror_handler

_el2_low_a32_sync:
    EL2_CALL_WITH_CONTEXT el2_low_a32_sync_handler

_el2_low_a32_irq:
    EL2_CALL_WITH_CONTEXT el2_low_a32_irq_handler

_el2_low_a32_fiq:
    EL2_CALL_WITH_CONTEXT el2_low_a32_fiq_handler

_el2_low_a32_serror:
    EL2_CALL_WITH_CONTEXT el2_low_a32_serror_handler


/* ------------------------------------------------ */
/*                  Vector table                    */
/* ------------------------------------------------ */

.macro SET_VENTRY label
.align 7
    b \label
.endm

.section .vectors, "ax"
.global _el2_vector_table
.align 11
_el2_vector_table:
//  0..3: Current EL, SP0
    SET_VENTRY _el2_cur_sp0_sync
    SET_VENTRY _el2_cur_sp0_irq
    SET_VENTRY _el2_cur_sp0_fiq
    SET_VENTRY _el2_cur_sp0_serror
//  4..7: Current EL, SPx 
    SET_VENTRY _el2_cur_spx_sync
    SET_VENTRY _el2_cur_spx_irq
    SET_VENTRY _el2_cur_spx_fiq
    SET_VENTRY _el2_cur_spx_serror
//  8..11: Lower EL, AArch64
    SET_VENTRY _el2_low_a64_sync
    SET_VENTRY _el2_low_a64_irq
    SET_VENTRY _el2_low_a64_fiq
    SET_VENTRY _el2_low_a64_serror
//  12..15: Lower EL, AArch32 
    SET_VENTRY _el2_low_a32_sync
    SET_VENTRY _el2_low_a32_irq
    SET_VENTRY _el2_low_a32_fiq
    SET_VENTRY _el2_low_a32_serror
