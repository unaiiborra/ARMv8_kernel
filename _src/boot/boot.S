#include <kernel/mm.h>

.section .text.boot
.global _start

_start:
    nop
    //  disable DAIF exceptions
    msr daifset, #0xf
    
    mrs x0, MPIDR_EL1
    and x0, x0, #0xFF // core Aff0

    ldr x3, =__NUM_CORES
    cmp x0, x3
    b.hi hang // if (coreid >= __NUM_CORES) hang

    ldr x1, =__EL2_STACK_SIZE
    ldr x2, =__stacks_el2_end
    mov x3, #KERNEL_BASE

    mul x0, x0, x1  // x0 = coreid * stack size
    sub x0, x2, x0  // x0 = stack end - coreid * stack size
    sub x0, x0, x3  // x0 = virtual_stack_pos - KERNEL_BASE
    
    mov sp, x0

    bl setup_system
#ifdef GDB
    nop
    nop
    nop
#endif
    adr x0, kernel_entry
    mov x1, #0  // ctx
    bl _switch_to_el1

setup_system:
    stp x29, x30, [sp, #-16]!

    mov x0, #0
    msr sp_el0, x0

    bl _init_gpr
#ifdef TEST
    mrs x0, CurrentEL
    lsr x0, x0, #2 // x0 = x0 >> 2
    cmp x0, #2
    b.ne hang
#endif
    bl _setup_stack_el1
    bl _el2_init_vector //  Set EL2 vector table
    bl _clean_bss

    ldr x0, =__text_start
    ldr x1, =__heap_start
    mov x2, #KERNEL_BASE

    sub x0, x0, x2
    sub x1, x1, x2

    bl _cache_flush_range

    ldp x29, x30, [sp], #16

    ret

hang:
    wfe
    b hang