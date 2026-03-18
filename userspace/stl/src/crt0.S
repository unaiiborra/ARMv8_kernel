.section .text

.global _start
_start:
    bl main

    bl syscall_exit  

0:  nop
    b 0b

