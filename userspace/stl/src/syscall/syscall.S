.section .text

/*
extern int64 syscall(
    x0 arg0,
    x1 arg1,
    x2 arg2,
    x3 arg3,
    x4 arg4,
    x5 arg5,
    x6 syscall);
*/
.global syscall
syscall:
    mov x8, x6
    svc #0
    ret
