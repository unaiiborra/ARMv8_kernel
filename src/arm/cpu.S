.section .text

.align 4

.global _ARM_get_sp
_ARM_get_sp:
    mov x0, sp
    ret


