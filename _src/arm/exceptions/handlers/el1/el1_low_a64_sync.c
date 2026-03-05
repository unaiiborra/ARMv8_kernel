#include <arm/exceptions/handlers/handlers_macros.h>
#include <kernel/exception/handler.h>


void el1_low_a64_sync_handler(void)
{
    exception_handler_sync();
}