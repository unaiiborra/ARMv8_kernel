.section .text
.align 4

// x0: entry address for EL1
// x1: ctx
.global _switch_to_el1
_switch_to_el1:
    mov x8, x0 // save ret addr in x8
    mov x9, x1 // save ctx in x9

#ifdef DEBUG
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #2
    b.ne hang
#endif

// EL1 sysreg configurations
    mov x0, #(1 << 11)      
    msr SCTLR_EL1, x0

    // Enable simd instructions for EL1 and floating point
    mov x0, #(3 << 20)    // FPEN = 0b11
    msr CPACR_EL1, x0

    adr x0, _el1_vector_table
    msr VBAR_EL1, x0

    bl _setup_stack_el1

// EL2 sysreg configurations

    // aarch64 mode
    mov x0, #(1 << 31)
    msr HCR_EL2, x0

    mov x0, #0x3c5
    msr SPSR_EL2, x0

    // Disable virtual timer for EL1
    mov x0, #0
    msr CNTVOFF_EL2, x0

    msr ELR_EL2, x8

    mov x0, x9
    isb
    eret

#ifdef DEBUG
hang:
    wfi
    b hang
#endif
