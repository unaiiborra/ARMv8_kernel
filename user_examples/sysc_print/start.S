.section .text

.global _start
_start:
    bl main


// args -> x0-x5 
// ret  -> x0
.global _supervisor_call
_supervisor_call:
    svc #0
    ret