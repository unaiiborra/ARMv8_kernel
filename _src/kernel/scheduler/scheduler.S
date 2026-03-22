#include <arm/exceptions/ctx.h>

#ifndef ECTX_IMPL
.macro ectx_load reg_pt
.endm
.error "<arm/exceptions/exceptions.h> not imported"
#endif

.section .text

/*
void _scheduler_loop_cpu_enter(arm_ectx* el0_ctx, pre_sched_mode_ctx* el1_ctx);
typedef struct {
    gpr_t fp, lr;

    sysreg_t spsr_el1;
    sp_t sp_el1;
} pre_sched_mode_ctx;
*/
.global _scheduler_loop_cpu_enter
_scheduler_loop_cpu_enter:    
#ifdef DEBUG
// assert we are using sp_el1
    mrs x20, spsel
    tbnz x20, #0, 1f

0:  wfi
    b 0b
1:
#endif

    mrs x2, spsr_el1
    mov x4, sp
    
    stp x29, x30,   [x1]
    stp x2, x4,     [x1, #16]

    ectx_load x0
    eret

    

// void _scheduler_loop_cpu_exit(pre_sched_mode_ctx el1_ctx);
.global _scheduler_loop_cpu_exit
_scheduler_loop_cpu_exit:
#ifdef DEBUG
// assert we are using sp_el1
    mrs x20, spsel
    tbnz x20, #0, 1f

0:  wfi
    b 0b
1:
#endif

    ldp x29, x30,   [x0]
    ldp x2, x4,     [x0, #16]

    msr spsr_el1, x2
    mov sp, x4

    msr elr_el1, x30

    eret
    
