#include "panic_exception_handlers.h"

#include <arm/sysregs/sysregs.h>
#include <kernel/io/stdio.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>

static inline exception_reason_sysregs get_exception_reason_sysregs()
{
    return (exception_reason_sysregs) {
        .esr  = sysreg_read(esr_el1),
        .elr  = sysreg_read(elr_el1),
        .far  = sysreg_read(far_el1),
        .spsr = sysreg_read(spsr_el1),
    };
}


static void print_exception_src(panic_exception_src src)
{
    const char* msg = "<*>";

    switch (src) {
        case PANIC_EXCEPTION_CUR_SP0:
            msg = "current EL, SP0";
            break;
        case PANIC_EXCEPTION_CUR_SPX:
            msg = "current EL, SPx";
            break;
        case PANIC_EXCEPTION_LOW_A32:
            msg = "lower EL, AArch32";
            break;
        case PANIC_EXCEPTION_LOW_A64:
            msg = "lower EL, AArch64";
            break;
        default:
            break;
    }

    fkprintf(IO_STDPANIC, "exception source: %s\n", msg);
}


static void print_raw_sysregs(exception_reason_sysregs* sysregs)
{
    fkprintf(
        IO_STDPANIC,
        "sysregs:\n"
        "\tesr:    %p\n"
        "\telr:    %p\n"
        "\tfar:    %p\n"
        "\tspsr:   %p\n",
        sysregs->esr,
        sysregs->elr,
        sysregs->far,
        sysregs->spsr);
}


void handle_sync_panic(panic_exception_src src)
{
    print_exception_src(src);

    exception_reason_sysregs sysregs = get_exception_reason_sysregs();

    print_raw_sysregs(&sysregs);

    print_esr(&sysregs, PANIC_EXCEPTION_TYPE_SYNC);
}

void handle_irq_panic(panic_exception_src src)
{
    print_exception_src(src);

    exception_reason_sysregs sysregs = get_exception_reason_sysregs();

    print_raw_sysregs(&sysregs);
}

void handle_fiq_panic(panic_exception_src src)
{
    print_exception_src(src);

    exception_reason_sysregs sysregs = get_exception_reason_sysregs();

    print_raw_sysregs(&sysregs);
}

void handle_serror_panic(panic_exception_src src)
{
    print_exception_src(src);

    exception_reason_sysregs sysregs = get_exception_reason_sysregs();

    print_raw_sysregs(&sysregs);
}
