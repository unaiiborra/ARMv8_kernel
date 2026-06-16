#pragma once

#include "kernel/panic.h"


typedef struct {
    uint64_t esr;
    uint64_t elr;
    uint64_t far;
    uint64_t spsr;
} exception_reason_sysregs;


void handle_sync_panic(panic_exception_src src);

void handle_irq_panic(panic_exception_src src);

void handle_fiq_panic(panic_exception_src src);

void handle_serror_panic(panic_exception_src src);


void print_esr(exception_reason_sysregs* r, panic_exception_type type);
