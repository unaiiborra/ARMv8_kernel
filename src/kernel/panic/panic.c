#include "kernel/panic.h"

#include <arm/exceptions/exceptions.h>
#include <arm/mmu.h>
#include <kernel/io/stdio.h>
#include <lib/lock.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stdbool.h>
#include <stddef.h>

#include "kernel/smp.h"
#include "panic_exception/panic_exception_handlers.h"


typedef enum {
    PANIC_UNRECOVERABLE = 0,
    PANIC_RECOVERABLE   = 1,
} panic_recovery;


static void default_info_print(panic_info* info)
{
    printf("\n\r" ANSI_BG_RED "\n\r[PANIC CORE %d]\n\r", get_cpuid());

    char* reason;
    switch (info->reason) {
        case PANIC_REASON_EXCEPTION:
            reason = "exception";
            break;
        case PANIC_REASON_MANUAL_ABORT:
            reason = "manual abort";
            break;
        default:
            reason = "UNDEFINED PANIC REASON!\n\r";
    }

    printf("reason:  %s\n\r", reason);

    printf(

        "mmu:     %s\n\r",
        mmu_is_active() ? "enabled" : "disabled");

    printf("message: %s\n\r", info->message);


    const char* enabled  = "enabled";
    const char* disabled = "disabled";
#define ENABLED_STR(cond) cond ? enabled : disabled

    printf(

        "\n\rexception status:\n\r"
        "\tfiq:    %s\n\r"
        "\tirq:    %s\n\r"
        "\tserror: %s\n\r"
        "\tdebug:  %s\n\r",

        ENABLED_STR(info->exception_status.fiq),    //
        ENABLED_STR(info->exception_status.irq),    //
        ENABLED_STR(info->exception_status.serror), //
        ENABLED_STR(info->exception_status.debug)   //
    );
}


static void handle_exception_panic(panic_info* info)
{
    panic_exception_src  src  = info->info.exception.src;
    panic_exception_type type = info->info.exception.type;


    switch (type) {
        case PANIC_EXCEPTION_TYPE_SYNC:
            handle_sync_panic(src);
            break;
        case PANIC_EXCEPTION_TYPE_IRQ:
            handle_irq_panic(src);
            break;
        case PANIC_EXCEPTION_TYPE_FIQ:
            handle_fiq_panic(src);
            break;
        case PANIC_EXCEPTION_TYPE_SERROR:
            handle_serror_panic(src);
            break;
    }
}


static void handle_manual_abort_panic(panic_info* info)
{
    /*
     *  lang
     */
    const char* lang_str;

    switch (info->info.manual_abort.lang) {
        case PANIC_LANG_ASM:
            lang_str = "asm";
            break;
        case PANIC_LANG_C:
            lang_str = "c";
            break;
        case PANIC_LANG_RUST:
            lang_str = "rust";
            break;
        default:
            lang_str = "undefined";
    }

    printf("language:%s\n\r", lang_str);

    /*
     *  file + line + col
     */
    panic_location location = info->info.manual_abort.location;

    printf("file:   %s\n\r", location.file);

    if (location.line >= 0)
        printf("line:   %d\n\r", location.line);
    if (location.col >= 0)
        printf("col:    %d\n\r", location.col);
}


static void handle_panic(panic_info* info, panic_recovery recovery)
{
    arm_exceptions_disable_all();

    cpulocked(IO_LOCK)
    {
        print("\n\r");


        if (!info)
            goto hang;

        default_info_print(info);

        switch (info->reason) {
            case PANIC_REASON_UNDEFINED:
                recovery = PANIC_UNRECOVERABLE;
                break;
            case PANIC_REASON_EXCEPTION:
                print("\n\r[EXCEPTION INFO]\n\r");
                handle_exception_panic(info);
                break;
            case PANIC_REASON_MANUAL_ABORT:
                print("\n\r[ABORT INFO]\n\r");
                handle_manual_abort_panic(info);
                break;
        }

        print(ANSI_CLEAR);
    }

#ifdef IRQ_DRIVEN_KPRINT
    arm_exceptions_enable(false, true, false, false);
#endif

    if (recovery == PANIC_UNRECOVERABLE)
        loop hang : asm volatile("wfe");
}


noreturn __attribute__((cold)) void panic(panic_info panic_info)
{
    handle_panic(&panic_info, PANIC_UNRECOVERABLE);

    // https://stackoverflow.com/questions/3381544/how-to-hint-to-gcc-that-a-line-should-be-unreachable-at-compile-time
    __builtin_unreachable();
}

void __attribute__((cold)) panicr(panic_info panic_info)
{
    handle_panic(&panic_info, PANIC_RECOVERABLE);
}
