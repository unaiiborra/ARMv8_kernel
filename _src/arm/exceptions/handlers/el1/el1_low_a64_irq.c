#include <kernel/exception/irq.h>


void el1_low_a64_irq_handler(void)
{
    irq_dispatch();
}
