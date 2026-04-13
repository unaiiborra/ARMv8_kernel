#include <kernel/mm.h>

.section .text

.align 4

.macro shutdown
    ldr x0, =0x84000008
    b _smc_call
.endm


// should enter with x0 having the context_id
.global _smp_wakeup_entry
_smp_wakeup_entry: 
    msr daifset, #0xf
    
    mrs x1, MPIDR_EL1
    and x1, x1, #0xFF // core Aff0

    ldr x4, =__NUM_CORES
    cmp x1, x4
    b.hi hang // if (coreid >= __NUM_CORES) hang

    ldr x2, =__EL2_STACK_SIZE
    ldr x3, =__stacks_el2_end
    mov x4, #KERNEL_BASE

    mul x1, x1, x2  // x1 = coreid * stack size
    sub x1, x3, x1  // x1 = stack end - coreid * stack size
    sub x1, x1, x4  // x1 = virtual_stack_pos - KERNEL_BASE
    
    mov sp, x1

    str x0, [sp, #-16]! // save context_id in the stack

    bl _setup_stack_el1
    bl _el2_init_vector 

    adr x0, smp_cpu_cfg
    ldr x1, [sp], #16

    bl _switch_to_el1

hang:
    wfe
    b hang
