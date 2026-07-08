.section .text

.global _secondary_entry
_secondary_entry:
    mov x10, x0 // thid
    mov x11, x1 // arguments
    bl __stl_setup_stack

    mov x29, #0
    mov x30, #0

    mov x0, x10
    mov x1, x11
    b   secondary_entry
