#include "kernel/mm.h"

.section .text

.global _mm_kva_to_kpa
//  puintptr_t _mm_kva_to_kpa(vuintptr_t addr)
_mm_kva_to_kpa:
    mov x1, #KERNEL_BASE
    sub x0, x0, x1
    ret


.global _mm_kpa_to_kva
//  vuintptr_t _mm_kpa_to_kva(puintptr_t addr)
_mm_kpa_to_kva:
    mov x1, #KERNEL_BASE
    add x0, x0, x1
    ret

.global _reset_stack_and_branch
_reset_stack_and_branch:
    mov sp, x0
    mov x29, #0
#ifdef DEBUG
    blr x1
0:
    wfi
    b 0b
#else
    br x1
# endif
