.section .text
.align 4


.global _spin_try_lock
_spin_try_lock:
    ldaxr   w1, [x0]        // load-acquire exclusive
    cbnz    w1, 1f          // if locked -> fail

    mov     w2, #1
    stxr    w3, w2, [x0]    // attempt to store
    cbnz    w3, 1f          // store failed -> fail

    mov     x0, #1          // true (success)
    ret
1:
    clrex                   // clear exclusive state
    mov     x0, #0          // false (not locked)
    ret


.global _spin_lock
// x0 = spinlock_t *lock
_spin_lock:
    ldaxr   w1, [x0]        // load acquire exclusive
    cbnz    w1, sleep       // slock != 0 -> spin

    mov     w2, #1
    stxr    w3, w2, [x0]    // store exclusive (0 for success 1 for failure in w3)
    cbnz    w3, sleep

    ret

sleep:
    wfe
    b _spin_lock


.global _spin_unlock
// x0 = spinlock_t *lock
_spin_unlock:
#ifdef TEST
    ldr     w1, [x0]
    cmp     w1, #1
    b.ne    1f
#endif
    mov     w1, #0
    stlr    w1, [x0]
    
    sev     // wake up other cores
    ret

#ifdef TEST
1:  wfe
    b       1b
#endif

