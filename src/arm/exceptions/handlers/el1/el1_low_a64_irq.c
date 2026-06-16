#include <kernel/exception/irq.h>

#include "kernel/scheduler.h"


void el1_low_a64_irq_handler(arm_ctx* ectx)
{
    scheduler_ectx_store(ectx);

    irq_dispatch();

    scheduler_ectx_load(ectx);
}
