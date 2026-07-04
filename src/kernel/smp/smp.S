#include <kernel/mm.h>

.section .text

.align 4

.macro shutdown
    ldr x0, =0x84000008
    b _smc_call
.endm


.global _setup_secondary_core
_setup_secondary_core:
    mov x0, #(1 << 11)
    msr SCTLR_EL1, x0

    ldr x0, =__text_start
    ldr x1, =__heap_start
    and x0, x0, #~KERNEL_BASE
    and x1, x1, #~KERNEL_BASE
    bl _cache_flush_range

    bl setup_secondary_core_el2

    mov x0, #(3 << 20)    // FPEN = 0b11
    msr CPACR_EL1, x0

    adr x0, _el1_vector_table
    orr x0, x0, #KERNEL_BASE
    msr VBAR_EL1, x0

    bl _setup_stack_el1
    mrs x0, SP_EL1
    orr x0, x0, #KERNEL_BASE
    msr SP_EL1, x0

    // aarch64 mode
    mov x0, #(1 << 31)
    msr HCR_EL2, x0

    mov x0, #0x3c5
    msr SPSR_EL2, x0

    // Disable virtual timer for EL1
    mov x0, #0
    msr CNTVOFF_EL2, x0

    adr x0, setup_secondary_core_el1
    orr x0, x0, #0xFFFF800000000000
    msr ELR_EL2, x0
    isb
    
    eret
