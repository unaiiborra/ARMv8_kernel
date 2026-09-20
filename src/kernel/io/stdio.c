#include <kernel/devices/device.h>
#include <kernel/devices/driver_ops/serial.h>
#include <kernel/io/stdio.h>
#include <kernel/io/term.h>
#include <kernel/io/vfs_serial.h>
#include <kernel/mm.h>
#include <lib/data_structures/kvec.h>
#include <lib/lock.h>
#include <lib/string.h>
#include <stdarg.h>
#include <stddef.h>

static spinlock_t io_lock = SPINLOCK_INIT;
spinlock_t* const IO_LOCK = &io_lock;

#ifdef DEBUG
static spinlock_t debug_trace_lock = SPINLOCK_INIT;
spinlock_t* const DEBUG_TRACE_LOCK = &debug_trace_lock;
#endif

void io_init()
{
    const device_t*     primary_uart = device_get_primary(DEVICE_CLASS_SERIAL);
    driver_handle_t     uart_handle  = device_get_driver_handle(primary_uart);
    const serial_ops_t* uart_ops     = get_serial_ops(primary_uart);

    uart_ops->init(uart_handle);
    uart_ops->set_baud(uart_handle, 115200, 12000000);
    uart_ops->irq_enable(uart_handle);

    const char* cls = ANSI_CLS ANSI_HOME;
    while (*cls) {
        if (uart_ops->putc(uart_handle, *cls) >= 0)
            cls++;
    }
}

void print(const char* s)
{
#ifdef IRQ_DRIVEN_KPRINT // irq driven kernel print
    spinlocked_irqsave(&io_lock)
    {
        term_prints(vfs_serial_out_term_get(), s);
    }
#else // polling kernel print
    const device_t*     primary_uart = device_get_primary(DEVICE_CLASS_SERIAL);
    driver_handle_t     uart_handle  = device_get_driver_handle(primary_uart);
    const serial_ops_t* uart_ops     = get_serial_ops(primary_uart);

    cpulocked_irqsave(&io_lock) while (true)
    {
        if (*s == '\0')
            break;

        int32_t res = uart_ops->putc(uart_handle, *s);

        if (res >= 0)
            s++;
    }
#endif
}

void printf(const char* s, ...)
{
    va_list va;
    va_start(va, s);

    scoped_kvec(char) string = kvec_new(char);

    fmt_string(&string, s, va);

    print(kvec_data(&string));

    va_end(va);
}
