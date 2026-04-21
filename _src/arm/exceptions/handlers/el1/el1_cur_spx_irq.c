#include <kernel/exception/irq.h>


void el1_cur_spx_irq_handler(void)
{
    irq_dispatch();
}
