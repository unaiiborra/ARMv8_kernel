.section .text
.align 4


.global _irq_lock
// irqflags_t _irq_lock(void);
_irq_lock:
    mrs     x0, DAIF
    msr     DAIFSET, #0b10    // disable irq
    ret


.global _irq_unlock
// void _irqrestore(irqflags_t irqlock);
_irq_unlock:
    msr     DAIF, x0
    ret