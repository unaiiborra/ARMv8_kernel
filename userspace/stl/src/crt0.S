.section .text

.global _start
_start:
    bl main

    // TODO: syscall exit

    mov x1, #0
    ldr x1, [x1] // should segfault

0:  nop
    b 0b

