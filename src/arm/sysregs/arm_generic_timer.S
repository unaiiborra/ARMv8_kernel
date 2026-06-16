.section .text
.align 4

/* ============================================================
 * Global counter
 * ============================================================ */

.global _ARM_CNTFRQ_EL0_set
_ARM_CNTFRQ_EL0_set:
    msr CNTFRQ_EL0, x0
    isb
    ret

.global _ARM_CNTFRQ_EL0_get
_ARM_CNTFRQ_EL0_get:
    mrs x0, CNTFRQ_EL0
    ret

.global _ARM_CNTPCT_EL0_get
_ARM_CNTPCT_EL0_get:
    mrs x0, CNTPCT_EL0
    ret

.global _ARM_CNTVCT_EL0_get
_ARM_CNTVCT_EL0_get:
    mrs x0, CNTVCT_EL0
    ret


/* ============================================================
 * Physical timer (EL0/EL1)
 * ============================================================ */

.global _ARM_CNTP_CTL_EL0_set
_ARM_CNTP_CTL_EL0_set:
    msr CNTP_CTL_EL0, x0
    isb
    ret

.global _ARM_CNTP_CTL_EL0_get
_ARM_CNTP_CTL_EL0_get:
    mrs x0, CNTP_CTL_EL0
    ret

.global _ARM_CNTP_CVAL_EL0_set
_ARM_CNTP_CVAL_EL0_set:
    msr CNTP_CVAL_EL0, x0
    isb
    ret

.global _ARM_CNTP_CVAL_EL0_get
_ARM_CNTP_CVAL_EL0_get:
    mrs x0, CNTP_CVAL_EL0
    ret

.global _ARM_CNTP_TVAL_EL0_set
_ARM_CNTP_TVAL_EL0_set:
    msr CNTP_TVAL_EL0, x0
    isb
    ret

.global _ARM_CNTP_TVAL_EL0_get
_ARM_CNTP_TVAL_EL0_get:
    mrs x0, CNTP_TVAL_EL0
    ret


/* ============================================================
 * Virtual timer (EL0/EL1)
 * ============================================================ */

.global _ARM_CNTV_CTL_EL0_set
_ARM_CNTV_CTL_EL0_set:
    msr CNTV_CTL_EL0, x0
    isb
    ret

.global _ARM_CNTV_CTL_EL0_get
_ARM_CNTV_CTL_EL0_get:
    mrs x0, CNTV_CTL_EL0
    ret

.global _ARM_CNTV_CVAL_EL0_set
_ARM_CNTV_CVAL_EL0_set:
    msr CNTV_CVAL_EL0, x0
    isb
    ret

.global _ARM_CNTV_CVAL_EL0_get
_ARM_CNTV_CVAL_EL0_get:
    mrs x0, CNTV_CVAL_EL0
    ret

.global _ARM_CNTV_TVAL_EL0_set
_ARM_CNTV_TVAL_EL0_set:
    msr CNTV_TVAL_EL0, x0
    isb
    ret

.global _ARM_CNTV_TVAL_EL0_get
_ARM_CNTV_TVAL_EL0_get:
    mrs x0, CNTV_TVAL_EL0
    ret


/*************************************************************
 * Timer control at EL1
 *************************************************************/

.global _ARM_CNTKCTL_EL1_set
_ARM_CNTKCTL_EL1_set:
    msr CNTKCTL_EL1, x0
    isb
    ret

.global _ARM_CNTKCTL_EL1_get
_ARM_CNTKCTL_EL1_get:
    mrs x0, CNTKCTL_EL1
    ret


/*************************************************************
 * Timer control at EL2
 *************************************************************/

.global _ARM_CNTHCTL_EL2_set
_ARM_CNTHCTL_EL2_set:
    msr CNTHCTL_EL2, x0
    isb
    ret

.global _ARM_CNTHCTL_EL2_get
_ARM_CNTHCTL_EL2_get:
    mrs x0, CNTHCTL_EL2
    ret

.global _ARM_CNTVOFF_EL2_set
_ARM_CNTVOFF_EL2_set:
    msr CNTVOFF_EL2, x0
    isb
    ret

.global _ARM_CNTVOFF_EL2_get
_ARM_CNTVOFF_EL2_get:
    mrs x0, CNTVOFF_EL2
    ret


/*************************************************************
 * Secure physical timer (opcional)
 *************************************************************/

.global _ARM_CNTPS_CTL_EL1_set
_ARM_CNTPS_CTL_EL1_set:
    msr CNTPS_CTL_EL1, x0
    isb
    ret

.global _ARM_CNTPS_CTL_EL1_get
_ARM_CNTPS_CTL_EL1_get:
    mrs x0, CNTPS_CTL_EL1
    ret

.global _ARM_CNTPS_CVAL_EL1_set
_ARM_CNTPS_CVAL_EL1_set:
    msr CNTPS_CVAL_EL1, x0
    isb
    ret

.global _ARM_CNTPS_CVAL_EL1_get
_ARM_CNTPS_CVAL_EL1_get:
    mrs x0, CNTPS_CVAL_EL1
    ret

.global _ARM_CNTPS_TVAL_EL1_set
_ARM_CNTPS_TVAL_EL1_set:
    msr CNTPS_TVAL_EL1, x0
    isb
    ret

.global _ARM_CNTPS_TVAL_EL1_get
_ARM_CNTPS_TVAL_EL1_get:
    mrs x0, CNTPS_TVAL_EL1
    ret
