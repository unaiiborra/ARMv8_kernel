#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/io/stdio.h>
#include <kernel/io/term.h>
#include <kernel/mm.h>
#include <stdarg.h>
#include <stddef.h>

#include "../devices/device_map.h"
#include "lib/stdmacros.h"
#include "stdputc.h"

static term_handle stdout, stdwarn, stderr, stdpanic;

static term_handle* const STDIO_OUTPUTS[4] = {
    [IO_STDOUT] = &stdout,
    [IO_STDWARN] = &stdwarn,
    [IO_STDERR] = &stderr,
    [IO_STDPANIC] = &stdpanic,
};


void io_early_init()
{
    uart_early_init(UART2_BASE);

    for (size_t i = 0; i < ARRAY_LEN(STDIO_OUTPUTS); i++)
        term_new(pt_as_kpa(STDIO_OUTPUTS[i]), pt_as_kpa(STDIO_EARLY_PUTC));
}


void io_init()
{
    for (size_t i = 0; i < ARRAY_LEN(STDIO_OUTPUTS); i++) {
        term_delete(STDIO_OUTPUTS[i]);
        term_new(STDIO_OUTPUTS[i], STDIO_PUTC[i]);
    }
}


void io_flush(io_out io)
{
    term_flush(STDIO_OUTPUTS[io]);
}


void fkprintf(io_out io, const char* s, ...)
{
    va_list va;

    __builtin_va_start(va, s);

    term_printf(STDIO_OUTPUTS[io], s, va);

    va_end(va);
}


void fkprint(io_out io, const char* s)
{
    term_prints(STDIO_OUTPUTS[io], s);
}
