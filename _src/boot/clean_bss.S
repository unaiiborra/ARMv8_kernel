#include <kernel/mm.h>

.section .text
.global _clean_bss
.align 2

_clean_bss:
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    mov     x3, #KERNEL_BASE

    sub     x0, x0, x3
    sub     x1, x1, x3

    movi    v0.16b, #0
    movi    v1.16b, #0
    movi    v2.16b, #0
    movi    v3.16b, #0

#ifdef DEBUG
//  Check 64 byte align
    ands    xzr, x0, #63
    b.ne    hang

    ands    xzr, x1, #63
    b.ne    hang
#endif

1:
    cmp     x0, x1
    b.ge    2f      // pointer of __bss_start >= __bss_end

    st1     {v0.16b, v1.16b, v2.16b, v3.16b}, [x0], #64
    b       1b
2:
    ret

#ifdef DEBUG
hang:
    wfe
    b hang
#endif
