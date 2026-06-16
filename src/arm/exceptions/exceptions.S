.section .text
.align 2

.global _exceptions_get_DAIF
_exceptions_get_DAIF:
    mrs x0, DAIF
    ret

.global _fiq_exceptions_enable
_fiq_exceptions_enable:
    msr daifclr, #0x1
    isb
    ret

.global _irq_exceptions_enable
_irq_exceptions_enable:
    msr daifclr, #0x2
    isb
    ret

.global _serror_exceptions_enable
_serror_exceptions_enable:
    msr daifclr, #0x4
    isb
    ret

.global _debug_exceptions_enable
_debug_exceptions_enable:
    msr daifclr, #0x8
    isb
    ret

.global _fiq_exceptions_disable
_fiq_exceptions_disable:
    msr daifset, #0x1
    isb
    ret

.global _irq_exceptions_disable
_irq_exceptions_disable:
    msr daifset, #0x2
    isb
    ret

.global _serror_exceptions_disable
_serror_exceptions_disable:
    msr daifset, #0x4
    isb
    ret

.global _debug_exceptions_disable
_debug_exceptions_disable:
    msr daifset, #0x8
    isb
    ret

