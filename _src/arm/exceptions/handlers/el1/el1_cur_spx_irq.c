#include <kernel/exception/handler.h>


void el1_cur_spx_irq_handler(void)
{
    exception_handler_irq();
}
