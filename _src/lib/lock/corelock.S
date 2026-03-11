.section .text

#define UNLOCKED_VALUE 0xFFFFFFFF

// void _core_lock(volatile uint32_t* l);
.global _core_lock
_core_lock:
    mrs     x2, MPIDR_EL1
    and     w2, w2, #0xFF
    sevl

0:  // try
    ldaxr   w1, [x0]

    mov     w4, #UNLOCKED_VALUE
    cmp     w1, w4
    b.eq    1f

    cmp     w1, w2
    b.eq    2f

    b       3f

1:  // lock
    stxr    w3, w2, [x0]    // store exclusive (0 for success 1 for failure in w3)
    cbnz    w3, 3f
    ret

2:  // return
    clrex
    ret

3:  // sleep
    wfe
    b       0b


// void _core_unlock(volatile uint32_t* l);
.global _core_unlock
_core_unlock:
    mrs     x2, MPIDR_EL1
    and     w2, w2, #0xFF

    ldr     w1, [x0]
    cmp     w1, w2
    b.ne    0f

    mov     w4, #UNLOCKED_VALUE
    mov     w1, w4
    stlr    w1, [x0]

    sev
    ret

0:  // PANIC: attempted to unlock a lock from another thread than the one that locked it
    wfi
    b       0b


// void _core_try_lock(volatile uint32_t* l);
.global _core_try_lock
_core_try_lock:
    mrs     x2, MPIDR_EL1
    and     w2, w2, #0xFF

0:  // try
    ldaxr   w1, [x0]

    mov     w4, #UNLOCKED_VALUE
    cmp     w1, w4
    b.eq    1f

    cmp     w1, w2
    b.eq    2f

    b       3f

1:  // lock
    stxr    w3, w2, [x0]    // store exclusive (0 for success 1 for failure in w3)
    cbnz    w3, 3f
    mov     x0, #1
    ret

2:  // already taken
    clrex
    mov     x0, #1
    ret

3:  // not taken
    clrex
    mov     x0, #0
    ret