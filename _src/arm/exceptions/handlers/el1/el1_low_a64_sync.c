#include <arm/exceptions/ctx.h>
#include <arm/exceptions/handlers/handlers_macros.h>
#include <kernel/exception/handler.h>
#include <kernel/io/stdio.h>
#include <stddef.h>

void el1_low_a64_sync_handler(arm_ctx* ectx)
{
    exception_handler_sync(ectx);
}