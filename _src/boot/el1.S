// https://github.com/antmicro/u-boot/blob/master/arch/arm/cpu/armv8/cache_v8.c#L133

.section .text
.align 4

// x0: entry address for el1
// x1: ctx
.global _switch_to_el1
_switch_to_el1:
    mov x8, x0 // save ret addr in x8
    mov x9, x1 // save ctx in x9

    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #2
    b.ne hang


    /* EL1 AArch64 */
    /*
    https://arm.jonpalmisc.com/latest_sysreg/AArch64-hcr_el2#fieldset_0-31_31-1 
    */

    mov x0, #(1 << 31)
    msr HCR_EL2, x0

    adr x0, _el1_vector_table
    msr VBAR_EL1, x0

    mov x0, #0x3c5
    msr SPSR_EL2, x0

    // Enable simd instructions for EL1 and floating point
    mov x0, #(3 << 20)    // FPEN = 0b11
    msr CPACR_EL1, x0

    // Disable virtual timer for EL1
    mov x0, #0
    msr CNTVOFF_EL2, x0

    adr x0, _el1_entry
    msr ELR_EL2, x0

    dsb sy
    dsb ish
    isb

    eret


// x8: entry address for el1
// x9: ctx
_el1_entry:
    nop
    mov x0, #1
    msr spsel, x0

    mov x0, #0x00            
    msr MAIR_EL1, x0

    mov x0, #(16)            
    msr TCR_EL1, x0

    mov x0, #(1 << 11)      
    msr SCTLR_EL1, x0
    isb

    mov x0, x9 // set ctx as arg0
    br x8   

hang:
    wfi
    b hang