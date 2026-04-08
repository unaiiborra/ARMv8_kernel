.section .text

.global _start
_start:
// TODO: provide args
    bl main


//  SYSC_EXIT
    mov x8, #0  
    svc #0
