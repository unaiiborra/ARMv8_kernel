.section .text
.align 4


.global spinlock_acquire_irqsave
// x0 = spinlock_t *lock
// return uint64_t flags
spinlock_acquire_irqsave:
    mrs     x4, DAIF
    msr     DAIFSET, #0b10    // disable irq
    
    stp     x29, x30, [sp, #-16]!
    bl      _spin_lock
    ldp     x29, x30, [sp], #16

    mov     x0, x4
    ret


.global _spinlock_release_irqrestore
// x0 = spinlock_t *lock
// x1 = uint64_t flags
_spinlock_release_irqrestore:
    mov     x4, x1
    
    stp     x29, x30, [sp, #-16]!
    bl      _spin_unlock
    ldp     x29, x30, [sp], #16

    msr     DAIF, x4            // restore daif
    ret
#ifdef TEST
0:  wfe
    b       0b
#endif