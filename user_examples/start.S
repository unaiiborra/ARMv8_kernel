.section .text


.global _start
_start:
    mov x0, #0xA
    mov x1, #0xB
    mov x2, #0xC
    mov x3, #0xD
    mov x4, #0xE
    mov x5, #0xF

    brk #0

hang:
    wfe
    b   hang