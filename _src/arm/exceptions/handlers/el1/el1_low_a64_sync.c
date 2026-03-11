#include <arm/exceptions/handlers/handlers_macros.h>
#include <kernel/exception/handler.h>
#include <kernel/io/stdio.h>
#include <stddef.h>
#include <stdint.h>

void el1_low_a64_sync_handler(arm_exception_ctx* ectx)
{
    exception_handler_sync(ectx);
}