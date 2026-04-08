#include "arm/exceptions/handlers/handlers_macros.h"

panic_exception_src select_src_enum_(const char* origin, const char* stack)
{
    if (strcmp(origin, "cur")) {
        if (strcmp(stack, "sp0"))
            return PANIC_EXCEPTION_CUR_SP0;

        if (strcmp(stack, "spx"))
            return PANIC_EXCEPTION_CUR_SPX;
    }

    if (strcmp(origin, "low")) {
        if (strcmp(stack, "a32"))
            return PANIC_EXCEPTION_LOW_A32;

        if (strcmp(stack, "a64"))
            return PANIC_EXCEPTION_LOW_A64;
    }

    PANIC("non valid configuration from panic exception handler call");
}


panic_exception_type select_exception_type_enum_(const char* type)
{
    if (strcmp(type, "sync"))
        return PANIC_EXCEPTION_TYPE_SYNC;

    if (strcmp(type, "irq"))
        return PANIC_EXCEPTION_TYPE_IRQ;

    if (strcmp(type, "fiq"))
        return PANIC_EXCEPTION_TYPE_FIQ;

    if (strcmp(type, "serror"))
        return PANIC_EXCEPTION_TYPE_SERROR;

    PANIC("non valid configuration from panic exception handler call");
}


void exception_panic(
    const char*          msg,
    panic_exception_src  src,
    panic_exception_type type)
{
    panic((panic_info) {
        .reason           = PANIC_REASON_EXCEPTION,
        .message          = msg,
        .exception_status = arm_exceptions_get_status(),
        .info             = {
            .exception = {
                .src  = src,
                .type = type,
            }}});
}
