#include "stdputc.h"

#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/io/stdio.h>
#include <kernel/io/term.h>
#include <stdint.h>

#include "kernel/devices/device.h"
#include "kernel/devices/driver_ops/serial.h"
#include "lib/branch.h"


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


static int32_t std_putc(const char c)
{
    const device_t*     io_device  = device_get_primary(DEVICE_CLASS_SERIAL);
    const serial_ops_t* serial_ops = io_device->driver_ops;
    driver_handle_t     handle     = device_get_driver_handle(io_device);

    return serial_ops->putc(handle, c);
}


static int32_t panic_putc(const char c)
{
    const device_t*     io_device  = device_get_primary(DEVICE_CLASS_SERIAL);
    const serial_ops_t* serial_ops = io_device->driver_ops;
    driver_handle_t     handle     = device_get_driver_handle(io_device);

    int32_t res;

    do {
        res = serial_ops->putc(handle, c);
    } while (res < 0);

    if (unlikely(c == '\n'))
        do {
            res = serial_ops->putc(handle, '\r');
        } while (res < 0);

    return 0;
}


const term_out STDIO_EARLY_PUTC = panic_putc;

const term_out STDIO_PUTC[4] = {
    [IO_STDOUT]   = std_putc,
    [IO_STDWARN]  = std_putc,
    [IO_STDERR]   = std_putc,
    [IO_STDPANIC] = panic_putc,
};
