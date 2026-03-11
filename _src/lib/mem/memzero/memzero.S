.section .text


// void* _memzero(v_uintptr_t dst, size_t size)
.global _memzero
.align 4
_memzero:
    mov     x3, x0

    cmp     x1, #16
    b.lt    tail8

//  Alignment: x4 = misalign
    ands    x4, x0, #15     // x4 = dst % 16
    b.eq    aligned         // if (x4 == 0) goto aligned;
    
    mov     x5, #16
    sub     x4, x5, x4      // x4 = 16 - x4 

    cmp     x4, x1
    csel    x4, x1, x4, hi  // x4 = min(misalign, size)

    sub     x1, x1, x4

    mov     x5, #0
    movi    v0.16b, #0
    movi    v1.16b, #0
    movi    v2.16b, #0
    movi    v3.16b, #0

head8:
    cmp     x4, #8
    b.lt    head4

    str     x5, [x0], #8
    sub     x4, x4, #8

head4:
    cmp     x4, #4
    b.lt    head2
h4:
    str     x5, [x0], #4
    sub    x4, x4, #4

head2:
    cmp     x4, #2
    b.lt    head1
h2:
    strh    w5, [x0], #2
    sub    x4, x4, #2

head1:
    cbz     x4, aligned
h1:
    strb    w5, [x0], #1

    sub     x4, x4, #1 // TODO: check if needed loop for head1
    cbnz    x4, hang

aligned:
    // TODO: delete after testing
    ands    xzr, x0, #15
    b.ne    hang  // only for debug, if not aligned to 16 hang, to verify with gdb

    cmp     x1, #64
    b.lt    tail32

loop:
    st1     {v0.16b-v3.16b}, [x0], #64

    sub     x1, x1, #64
    cmp     x1, #64
    b.ge    loop
    b       tail8

tail32:
    cmp     x1, #32
    b.lt    tail8

    st1     {v0.16b, v1.16b}, [x0], #32

    sub     x1, x1, #32

tail8:      // tail8 loops as for example if the size 63 is requested 63 - 32 -> 31, and then with 8 loops the result is near for tail4 or tail1
    cmp     x1, #8
    b.lt    tail4
t8:
    str     x5, [x0], #8

    sub     x1, x1, #8
    cmp     x1, #8
    b.ge    t8

tail4:
    cmp     x1, #4
    b.lt    tail1
t4:
    str     w5, [x0], #4

    sub     x1, x1, #4

tail1:
    cbz     x1, return
t1:
    strb    w5, [x0], #1

    subs    x1, x1, #1
    b.ne    t1

return:
    mov     x0, x3
    ret

hang:
    wfe
    b hang



// void* _memzero(v_uintptr_t dst, size_t size)
.global  _memzero64
_memzero64:
    cbz x1, 1f

#ifdef DEBUG
    ands    xzr, x0, #15     // x4 = dst % 16
    b.ne    hang             // if (x4 != 0) goto hang;

    ands    xzr, x1, #63
    b.ne    hang

    cmp     x1, #64
    b.lt    hang
#endif

    mov     x2, x0

    movi    v0.16b, #0
    movi    v1.16b, #0
    movi    v2.16b, #0
    movi    v3.16b, #0

0:
    st1     {v0.16b-v3.16b}, [x0], #64
    subs     x1, x1, #64
    b.gt    0b

    mov     x0, x2
1:
    ret
