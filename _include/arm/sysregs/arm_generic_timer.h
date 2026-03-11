#pragma once
#include <stddef.h>
#include <stdint.h>
/* ============================================================
 * Global counter
 * ============================================================ */

/* Counter-timer Frequency Register */
extern uint64_t _ARM_CNTFRQ_EL0_get(void);

/* Physical Count Register */
extern uint64_t _ARM_CNTPCT_EL0_get(void);

/* Virtual Count Register */
extern uint64_t _ARM_CNTVCT_EL0_get(void);

/* ============================================================
 * Physical timer (EL0/EL1)
 * ============================================================ */

/* Physical Timer Control */
extern void _ARM_CNTP_CTL_EL0_set(uint64_t v);
extern uint64_t _ARM_CNTP_CTL_EL0_get(void);

/* Physical Timer Compare Value */
extern void _ARM_CNTP_CVAL_EL0_set(uint64_t v);
extern uint64_t _ARM_CNTP_CVAL_EL0_get(void);

/* Physical Timer Timer Value */
extern void _ARM_CNTP_TVAL_EL0_set(uint64_t v);
extern uint64_t _ARM_CNTP_TVAL_EL0_get(void);

/* ============================================================
 * Virtual timer (EL0/EL1)
 * ============================================================ */

/* Virtual Timer Control */
extern void _ARM_CNTV_CTL_EL0_set(uint64_t v);
extern uint64_t _ARM_CNTV_CTL_EL0_get(void);

/* Virtual Timer Compare Value */
extern void _ARM_CNTV_CVAL_EL0_set(uint64_t v);
extern uint64_t _ARM_CNTV_CVAL_EL0_get(void);

/* Virtual Timer Timer Value */
extern void _ARM_CNTV_TVAL_EL0_set(uint64_t v);
extern uint64_t _ARM_CNTV_TVAL_EL0_get(void);

/* ============================================================
 * Timer control at EL1
 * ============================================================ */

/* Counter-timer Kernel Control Register */
extern void _ARM_CNTKCTL_EL1_set(uint64_t v);
extern uint64_t _ARM_CNTKCTL_EL1_get(void);

/* ============================================================
 * Timer control at EL2 (hypervisor)
 * ============================================================ */

/* Counter-timer Hypervisor Control Register */
extern void _ARM_CNTHCTL_EL2_set(uint64_t v);
extern uint64_t _ARM_CNTHCTL_EL2_get(void);

/* Virtual Counter Offset Register */
extern void _ARM_CNTVOFF_EL2_set(uint64_t v);
extern uint64_t _ARM_CNTVOFF_EL2_get(void);
