#include "kernel/mm.h"

.section .text

.global _get_pc
_get_pc:
    mov x0, x30
    ret


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