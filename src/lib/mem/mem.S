.section .text


.global _ID_AA64MMFR0_EL1_read
_ID_AA64MMFR0_EL1_read:
    mrs x0, ID_AA64MMFR0_EL1
    ret