#include <arm/exceptions/exceptions.h>
#include <arm/sysregs/sysregs.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>
extern uint64_t _exceptions_get_DAIF();

extern void _fiq_exceptions_enable();
extern void _irq_exceptions_enable();
extern void _serror_exceptions_enable();
extern void _debug_exceptions_enable();

extern void _fiq_exceptions_disable();
extern void _irq_exceptions_disable();
extern void _serror_exceptions_disable();
extern void _debug_exceptions_disable();

// https://developer.arm.com/documentation/111107/2025-09/AArch64-Registers/DAIF--Interrupt-Mask-Bits
arm_exception_status arm_exceptions_get_status()
{
    uint64_t daif = _exceptions_get_DAIF();

    return (arm_exception_status) {.fiq = !((daif >> 6) & 1UL),
                                   .irq = !((daif >> 7) & 1UL),
                                   .serror = !((daif >> 8) & 1UL),
                                   .debug = !((daif >> 9) & 1UL)};
}

void arm_exceptions_set_status(arm_exception_status status)
{
    if (status.fiq)
        _fiq_exceptions_enable();
    else
        _fiq_exceptions_disable();

    if (status.irq)
        _irq_exceptions_enable();
    else
        _irq_exceptions_disable();

    if (status.serror)
        _serror_exceptions_enable();
    else
        _serror_exceptions_disable();

    if (status.debug)
        _debug_exceptions_enable();
    else
        _debug_exceptions_disable();

#ifdef TEST
    arm_exception_status current = arm_exceptions_get_status();

    if (current.fiq != status.fiq || current.irq != status.irq ||
        current.serror != status.serror || current.debug != status.debug)
        PANIC("exceptions_set_status error");
#endif
}

void arm_exceptions_enable(bool fiq, bool irq, bool serror, bool debug)
{
    if (fiq)
        _fiq_exceptions_enable();

    if (irq)
        _irq_exceptions_enable();

    if (serror)
        _serror_exceptions_enable();

    if (debug)
        _debug_exceptions_enable();
}

void arm_exceptions_disable(bool fiq, bool irq, bool serror, bool debug)
{
    if (fiq)
        _fiq_exceptions_disable();

    if (irq)
        _irq_exceptions_disable();

    if (serror)
        _serror_exceptions_disable();

    if (debug)
        _debug_exceptions_disable();
}

size_t arm_get_exception_level()
{
    return (size_t)sysreg_read(currentel);
}
