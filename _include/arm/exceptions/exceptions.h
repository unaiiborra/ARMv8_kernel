#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    bool fiq;
    bool irq;
    bool serror;
    bool debug;
} arm_exception_status;

/// true means enabled
arm_exception_status arm_exceptions_get_status();

/// true means enable, false disable
void arm_exceptions_set_status(arm_exception_status status);

/// Enables exceptions on true. If a param is false, the current state of the
/// exception will be mantained, not disabled.
void arm_exceptions_enable(bool fiq, bool irq, bool serror, bool debug);

/// Disables exceptions on true. If a param is false, the current state of the
/// exception will be mantained, not enabled.
void arm_exceptions_disable(bool fiq, bool irq, bool serror, bool debug);

size_t arm_get_exception_level();
