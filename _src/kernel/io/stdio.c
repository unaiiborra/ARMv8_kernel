#include <drivers/imx8mp_uart.h>
#include <kernel/io/stdio.h>
#include <kernel/io/term.h>
#include <kernel/mm.h>
#include <lib/string.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/devices/device.h"
#include "kernel/lib/kvec.h"
#include "lib/lock.h"
#include "lib/stdattribute.h"


void io_init()
{
    const device_t*     primary_uart = device_get_primary(DEVICE_CLASS_SERIAL);
    driver_handle_t     uart_handle  = device_get_driver_handle(primary_uart);
    const serial_ops_t* uart_ops     = get_serial_ops(primary_uart);

    uart_ops->init(uart_handle);
    uart_ops->set_baud(uart_handle, 115200, 12000000);
    uart_ops->irq_enable(uart_handle);
}


void print(const char* s)
{
    const device_t*     primary_uart = device_get_primary(DEVICE_CLASS_SERIAL);
    driver_handle_t     uart_handle  = device_get_driver_handle(primary_uart);
    const serial_ops_t* uart_ops     = get_serial_ops(primary_uart);

    irqlocked() while (true)
    {
        if (*s == '\0')
            break;

        int32_t res = uart_ops->putc(uart_handle, *s);

        if (res >= 0)
            s++;
    }
}


static void putfmt(char c, void* string)
{
    kvec_push(string, &c);
}

void printf(const char* s, ...)
{
    va_list va;
    va_start(va, s);

    scoped_kvec(char) string = kvec_new(char);

    str_fmt_print(putfmt, &string, s, va);

    print(kvec_data(&string));

    va_end(va);
}
