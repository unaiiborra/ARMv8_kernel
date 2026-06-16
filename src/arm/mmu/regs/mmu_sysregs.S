.section .text
.align 4

.global _mmu_get_MAIR_EL1
_mmu_get_MAIR_EL1:
    mrs x0, MAIR_EL1
    ret

.global _mmu_set_MAIR_EL1
_mmu_set_MAIR_EL1:
    ic iallu
    dsb sy
    isb

    tlbi vmalle1
    dsb sy
    isb

    msr MAIR_EL1, x0

    ic iallu
    dsb sy
    isb

    tlbi vmalle1
    dsb sy
    isb

    ret


/* -------------------------------------------------- */
/* SCTLR_EL1                                          */
/* -------------------------------------------------- */
.global _mmu_get_SCTLR_EL1
_mmu_get_SCTLR_EL1:
    mrs x0, SCTLR_EL1
    ret

.global _mmu_set_SCTLR_EL1
_mmu_set_SCTLR_EL1:
    msr SCTLR_EL1, x0
    isb            
    ret


/* -------------------------------------------------- */
/* TTBR0_EL1                                         */
/* -------------------------------------------------- */
.global _mmu_get_TTBR0_EL1
_mmu_get_TTBR0_EL1:
    mrs x0, TTBR0_EL1
    ret

.global _mmu_set_TTBR0_EL1
_mmu_set_TTBR0_EL1:
    msr TTBR0_EL1, x0
    dsb ish         
    isb
    ret


/* -------------------------------------------------- */
/* TTBR1_EL1                                         */
/* -------------------------------------------------- */
.global _mmu_get_TTBR1_EL1
_mmu_get_TTBR1_EL1:
    mrs x0, TTBR1_EL1
    ret

.global _mmu_set_TTBR1_EL1
_mmu_set_TTBR1_EL1:
    msr TTBR1_EL1, x0
    dsb ish
    isb
    ret


/* -------------------------------------------------- */
/* TCR_EL1                                           */
/* -------------------------------------------------- */
.global _mmu_get_TCR_EL1
_mmu_get_TCR_EL1:
    mrs x0, TCR_EL1
    ret

.global _mmu_set_TCR_EL1
_mmu_set_TCR_EL1:
    msr TCR_EL1, x0
    isb            
    ret


.global _mmu_get_ID_AA64MMFR0_EL1
_mmu_get_ID_AA64MMFR0_EL1:
    mrs x0, ID_AA64MMFR0_EL1
    ret


