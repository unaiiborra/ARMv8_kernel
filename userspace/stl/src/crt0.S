
#if !defined(STACK_SIZE) || STACK_SIZE % 4096 != 0
#   error "STACK_SIZE must be a multiple of PAGE_SIZE"
#endif

.section .text.boot
.global _start
_start:
    mov x9, x0
    bl __stl_setup_stack
    mov x0, x9
// TODO: provide args
    bl main

//  SYSC_EXIT
    mov x8, #0
    svc #0

.section .text.stl
.global __stl_error_exit
__stl_error_exit:
    mov x8, #0
    mov x0, #1
    svc #0

.global __stl_setup_stack
__stl_setup_stack:
    ldr x1, =STACK_SIZE
    cbz x1, 1f

    mov x0, #0
    mov x2, #0x3 // RW
    mov x3, #1   // ANONYMOUS
    mov x4, #0
    mov x5, #0
    mov x8, #5   // MMAP
    svc #0

    cmp x0, #0
    b.ge 0f
    b __stl_error_exit

0:  ldr x1, =STACK_SIZE
    add x0, x0, x1
    mov sp, x0
1:
    ret
