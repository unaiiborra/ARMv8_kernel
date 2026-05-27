#include <kernel/mm.h>

.section .text

.align 4
.global _setup_stack_el1
_setup_stack_el1:
    mrs x0, MPIDR_EL1
    and x0, x0, #0xFF // core Aff0

    ldr x3, =__NUM_CORES
    cmp x0, x3
    b.hi hang // if (coreid >= __NUM_CORES) hang

    ldr x1, =__EL1_STACK_SIZE
    ldr x2, =__stacks_el1_end

    mul x0, x0, x1  // x0 = coreid * stack size
    sub x0, x2, x0  // x0 = stack end - coreid * stack size
    and x0, x0, #~KERNEL_BASE
    
    msr SP_EL1, x0
    ret



.global _reset_stack_el1
_reset_stack_el1:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #1
    b.ne hang

    mrs x0, MPIDR_EL1
    and x0, x0, #0xFF // core Aff0

    ldr x3, =__NUM_CORES
    cmp x0, x3
    b.hi hang // if (coreid >= __NUM_CORES) hang

    ldr x1, =__EL1_STACK_SIZE
    ldr x2, =__stacks_el1_end

    mul x0, x0, x1  // x0 = coreid * stack size
    sub x0, x2, x0  // x0 = stack end - coreid * stack size
    
    mov sp, x0
    isb
    ret
hang:
    wfe
    b hang
