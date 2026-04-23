#pragma once

#include <arm/exceptions/ctx.h>

void exception_handler_irq();
void exception_handler_sync(arm_ctx* ectx);