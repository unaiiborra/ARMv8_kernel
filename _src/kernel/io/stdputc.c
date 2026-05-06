#include "stdputc.h"

#include <drivers/imx8mp_uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/io/stdio.h>
#include <kernel/io/term.h>
#include <stdint.h>

#include "kernel/devices/device.h"
#include "kernel/devices/driver_ops/serial.h"
#include "lib/branch.h"



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
