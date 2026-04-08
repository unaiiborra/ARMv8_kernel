#include "stdputc.h"

#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/io/stdio.h>
#include <kernel/io/term.h>


#define ANSI_BG_RED "\x1b[41m"
#define ANSI_CLEAR  "\x1b[0m"
#define ANSI_CLS    "\x1b[2J"
#define ANSI_HOME   "\x1b[H"

#define ANSI_SAVE_CURSOR        "\x1b[s"
#define ANSI_RESTORE_CURSOR_POS "\x1b[u"

#define ANSI_MOVE_CURSOR_RIGHT(n) "\x1b[" #n "C"
#define ANSI_MOVE_CURSOR_LEFT(n)  "\x1b[" #n "D"

#define ANSI_ERASE_FROM_CURSOR_TO_END_OF_SCREEN "\x1b[0J"
#define ANSI_ERASE_LINE                         "\x1b[K"


static term_out_result early_putc(const char c)
{
    return uart_putc_early(c);
}


static term_out_result std_putc(const char c)
{
    return uart_putc(&UART2_DRIVER, c);
}


static term_out_result panic_putc(const char c)
{
    if (uart_putc_sync(&UART2_DRIVER, c) == TERM_OUT_RES_NOT_TAKEN)
        goto hang;

    if (c == '\n') {
        const char* str = "\r" ANSI_ERASE_LINE;

        while (*str)
            if (uart_putc_sync(&UART2_DRIVER, *str++) == TERM_OUT_RES_NOT_TAKEN)
                goto hang;
    }

    return TERM_OUT_RES_OK;

hang:
    while (true)
        asm volatile("wfi");
}


const term_out STDIO_EARLY_PUTC = early_putc;

const term_out STDIO_PUTC[4] = {
    [IO_STDOUT]   = std_putc,
    [IO_STDWARN]  = std_putc,
    [IO_STDERR]   = std_putc,
    [IO_STDPANIC] = panic_putc,
};
