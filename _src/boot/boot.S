#include <kernel/mm.h>

.section .text.boot
.global _start

_start:
    //  disable DAIF exceptions
    msr daifset, #0xf

    msr spsel, #1       // select SP_EL2 as sp
    isb

    //  setup EL2 stack
    mrs x0, MPIDR_EL1
    and x0, x0, #0xFF   // core Aff0
    ldr x1, =__EL2_STACK_SIZE
    ldr x2, =__stacks_el2_end

    mul x0, x0, x1      // x0 = coreid * stack size
    sub x0, x2, x0      // x0 = stack end - coreid * stack size
    and x0, x0, #~KERNEL_BASE      // x0 = virtual_stack_pos & ~KERNEL_BASE
    
    mov sp, x0
    mov x0, #0
    msr sp_el0, x0


    bl _init_gpr
    bl _el2_init_vector //  Set EL2 vector table
    bl _setup_stack_el1
    bl _clean_bss


    ldr x0, =__text_start
    ldr x1, =__heap_start
    and x0, x0, #~KERNEL_BASE
    and x1, x1, #~KERNEL_BASE
    bl _cache_flush_range


    adr x0, kernel_entry
    mov x1, #0  // ctx
    b _switch_to_el1
