#pragma once

#include <arm/exceptions/ctx.h>
#include <kernel/panic.h>
#include <lib/string.h>


panic_exception_src  select_src_enum_(const char* origin, const char* stack);
panic_exception_type select_exception_type_enum_(const char* type);

void exception_panic(
    const char*          msg,
    panic_exception_src  src,
    panic_exception_type type);


/// Declares a non implemented EL2 exception handler, panics with the name of
/// the exception
#define DECLARE_EL2_EXCEPTION_HANDLER_PANIC(origin, stack, type) \
    void el2_##origin##_##stack##_##type##_handler(void)         \
    {                                                            \
        exception_panic(                                         \
            "el2_" #origin "_" #stack "_" #type "exception",     \
            select_src_enum_(#origin, #stack),                   \
            select_exception_type_enum_(#type));                 \
    }


/// Declares a non implemented EL1 exception handler, panics with the name of
/// the exception
#define DECLARE_EL1_EXCEPTION_HANDLER_PANIC(origin, stack, type) \
    void el1_##origin##_##stack##_##type##_handler(arm_ctx*)     \
    {                                                            \
        exception_panic(                                         \
            "el1_" #origin "_" #stack "_" #type " exception",    \
            select_src_enum_(#origin, #stack),                   \
            select_exception_type_enum_(#type));                 \
    }
