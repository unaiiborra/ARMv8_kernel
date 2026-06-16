#pragma once

#include <stddef.h>
#include <stdint.h>

extern uint64_t _mmu_get_MAIR_EL1(void);
extern void _mmu_set_MAIR_EL1(uint64_t v);

extern uint64_t _mmu_get_SCTLR_EL1(void);
extern void _mmu_set_SCTLR_EL1(uint64_t v);

extern uint64_t _mmu_get_TTBR0_EL1(void);
extern void _mmu_set_TTBR0_EL1(uint64_t v);

extern uint64_t _mmu_get_TTBR1_EL1(void);
extern void _mmu_set_TTBR1_EL1(uint64_t v);

// https://df.lth.se/~getz/ARM/SysReg/AArch64-tcr_el1.html#fieldset_0-5_0
extern uint64_t _mmu_get_TCR_EL1(void);
extern void _mmu_set_TCR_EL1(uint64_t v);

extern uint64_t _mmu_get_ID_AA64MMFR0_EL1(void);
