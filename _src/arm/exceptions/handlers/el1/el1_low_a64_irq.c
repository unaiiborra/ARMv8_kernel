#include <kernel/exception/handler.h>


void el1_low_a64_irq_handler(void)
{
    exception_handler_irq();
}
