.section .text

.align 4

.global _GICV3_ARM_ICC_SRE_EL1_write
_GICV3_ARM_ICC_SRE_EL1_write:
    msr ICC_SRE_EL1, x0
    isb
    ret

.global _GICV3_ARM_ICC_PMR_EL1_write
_GICV3_ARM_ICC_PMR_EL1_write:
    msr ICC_PMR_EL1, x0
    isb
    ret

.global _GICV3_ARM_ICC_PMR_EL1_read
_GICV3_ARM_ICC_PMR_EL1_read:
    mrs x0, ICC_PMR_EL1
    ret

.global _GICV3_ARM_ICC_IGRPEN1_EL1_write
_GICV3_ARM_ICC_IGRPEN1_EL1_write:
    msr ICC_IGRPEN1_EL1, x0
    isb
    ret

.global _GICV3_ARM_ICC_IAR1_EL1_read
_GICV3_ARM_ICC_IAR1_EL1_read:
    mrs x0, ICC_IAR1_EL1
    ret

.global _GICV3_ARM_ICC_EOIR1_EL1_write
_GICV3_ARM_ICC_EOIR1_EL1_write:
    msr ICC_EOIR1_EL1, x0
    ret