#pragma once

#include <arm/exceptions/ctx.h>
#include <drivers/interrupts/gicv3/gicv3.h>
#include <kernel/exception/interrupts.h>


void exception_handler_irq();
void exception_handler_sync(arm_ctx* ectx);