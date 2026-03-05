.section .text

.global _start
_start:
    mov x0, #0
    mov x1, #1
    svc #0

0:  nop
    b 0b
    ret