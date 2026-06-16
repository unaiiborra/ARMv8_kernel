.section .text

/*
    Standard memcpy 
*/

// void *_memcpy64(void *dst, void *src, uint64_t size);
.global _memcpy
.align 4
_memcpy:
    mov     x3, x0

    cmp     x2, #16
    b.lt    tail8

//  Alignment: x4 = misalign
    ands    x4, x0, #15     // x4 = dst % 16
    b.eq    aligned         // if (x4 == 0) goto aligned;
    
    mov     x5, #16
    sub     x4, x5, x4      // x4 = 16 - x4 

    cmp     x4, x2
    csel    x4, x2, x4, hi  // x4 = min(misalign, size)

    sub     x2, x2, x4

head8:
    cmp     x4, #8
    b.lt    head4

    ldr     x5, [x1], #8
    str     x5, [x0], #8
    sub     x4, x4, #8

head4:
    cmp     x4, #4
    b.lt    head2
h4:
    ldr     x5, [x1], #4
    str     x5, [x0], #4
    sub    x4, x4, #4

head2:
    cmp     x4, #2
    b.lt    head1
h2:
    ldrh    w5, [x1], #2
    strh    w5, [x0], #2
    sub    x4, x4, #2

head1:
    cbz     x4, aligned
h1:
    ldrb    w5, [x1], #1
    strb    w5, [x0], #1

    sub     x4, x4, #1 // TODO: check if needed loop for head1
    cbnz    x4, hang

aligned:
    // TODO: delete after testing
    ands    xzr, x0, #15
    b.ne    hang  // only for debug, if not aligned to 16 hang, to verify with gdb

    cmp     x2, #64
    b.lt    tail32

loop:
    ld1     {v0.16b-v3.16b}, [x1], #64
    st1     {v0.16b-v3.16b}, [x0], #64

    sub     x2, x2, #64
    cmp     x2, #64
    b.ge    loop
    b       tail8

tail32:
    cmp     x2, #32
    b.lt    tail8

    ld1     {v0.16b, v1.16b}, [x1], #32
    st1     {v0.16b, v1.16b}, [x0], #32

    sub     x2, x2, #32

tail8:      // tail8 loops as for example if the size 63 is requested 63 - 32 -> 31, and then with 8 loops the result is near for tail4 or tail1
    cmp     x2, #8
    b.lt    tail4
t8:
    ldr     x4, [x1], #8
    str     x4, [x0], #8

    sub     x2, x2, #8
    cmp     x2, #8
    b.ge    t8

tail4:
    cmp     x2, #4
    b.lt    tail1
t4:
    ldr     w4, [x1], #4
    str     w4, [x0], #4

    sub     x2, x2, #4

tail1:
    cbz     x2, return
t1:
    ldrb    w4, [x1], #1
    strb    w4, [x0], #1

    subs    x2, x2, #1
    b.ne    t1

return:
    mov     x0, x3
    ret

hang:
    wfe
    b hang


/*  
    -----------------------------------------
    The below functions are memcpys that copy
    in multiples of N, they dont check for 
    alignment. If the size is not a multiple 
    of N, the remainder is not copied.

    For all the fns below:

    x0:     dst
    x1:     src
    x2:     size
    x3:     copy of dst
    x4/w4:  copy of the data using ldrx/strx
    ----------------------------------------- 
*/

// void *_memcpy64(void *dst, void *src, uint64_t size);
.global _memcpy64
.align 4
_memcpy64:
    mov     x3, x0      // save return value (dst)

    cmp     x2, #64
    b.lt    return64
    
loop64:     
    ld1		{v0.16b-v3.16b}, [x1], #64
    st1     {v0.16b-v3.16b}, [x0], #64

    sub     x2, x2, #64
    cmp     x2, #64
    b.ge    loop64      // if (size >= 64) goto loop64;

return64:
    mov     x0, x3
    ret

// void *_memcpy32(void *dst, void *src, uint64_t size);
.global _memcpy32
.align 4
_memcpy32:
    mov     x3, x0

    cmp     x2, #32
    b.lt    return32

loop32:
    ld1     {v0.16b, v1.16b}, [x1], #32
    st1     {v0.16b, v1.16b}, [x0], #32

    sub     x2, x2, #32
    cmp     x2, #32
    b.ge    loop32

return32:
    mov     x0, x3
    ret

// void *_memcpy16(void *dst, void *src, uint64_t size);
.global _memcpy16
.align 4
_memcpy16:
    mov     x3, x0

    cmp     x2, #16
    b.lt    return16
loop16:
    ld1     {v0.16b}, [x1], #16
    st1     {v0.16b}, [x0], #16

    sub     x2, x2, #16
    cmp     x2, #16
    b.ge    loop16

return16:
    mov     x0,x3
    ret

// void *_memcpy8(void *dst, void *src, uint64_t size);
.global _memcpy8
.align 4
_memcpy8:
    mov     x3, x0

    cmp     x2, #8
    b.lt    return8

loop8:
    ldr     x4, [x1], #8
    str     x4, [x0], #8

    sub     x2, x2, #8
    cmp     x2, #8
    b.ge    loop8

return8:
    mov     x0, x3
    ret

// void *_memcpy4(void *dst, void *src, uint64_t size);
.global _memcpy4
.align 4
_memcpy4:
    mov     x3, x0

    cmp     x2, #4
    b.lt    return4

loop4:
    ldr     w4, [x1], #4
    str     w4, [x0], #4

    sub     x2, x2, #4
    cmp     x2, #4
    b.ge    loop4

return4:
    mov     x0, x3
    ret

// void *_memcpy2(void *dst, void *src, uint64_t size);
.global _memcpy2
.align 4
_memcpy2:
    mov     x3, x0

    cmp     x2, #2
    b.lt    return2

loop2:
    ldrh    w4, [x1], #2
    strh    w4, [x0], #2

    sub     x2, x2, #2
    cmp     x2, #2
    b.ge    loop2

return2:
    mov     x0, x3
    ret

// void *_memcpy1(void *dst, void *src, uint64_t size);
.global _memcpy1
.align 4
_memcpy1:
    mov     x3, x0

    cbz     x2, return1

loop1:
    ldrb    w4, [x1], #1
    strb    w4, [x0], #1

    subs    x2, x2, #1
    b.ne    loop1

return1:
    mov     x0, x3
    ret    
