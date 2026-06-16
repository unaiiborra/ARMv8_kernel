#include <arm/exceptions/ctx.h>

#ifndef ECTX_IMPL
.macro ectx_store
.endm
.macro ectx_load reg_pt
.endm
.error "<arm/exceptions/exceptions.h> not imported"
#endif


.section .text
.global _el1_init_vector
.align 4
_el1_init_vector:
    adr     x0, _el1_vector_table
    msr     VBAR_EL1, x0
    isb
    ret



.macro EL1_CALL_WITH_CONTEXT label
    ectx_store
    bl \label
    ectx_load sp
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
