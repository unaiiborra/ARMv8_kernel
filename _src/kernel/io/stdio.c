#include <drivers/imx8mp_uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/io/stdio.h>
#include <kernel/io/term.h>
#include <kernel/mm.h>
#include <lib/string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/devices/device.h"
#include "kernel/panic.h"
#include "lib/lock.h"
#include "lib/stdmacros.h"
#include "stdputc.h"

#define IO_COUNT 4


typedef struct {
    const char* device_name;   // NULL if primary
    bool       notify_enabled; // if the irq notification of tx ready is enabled
    spinlock_t lock;           // locks notify_enabled
    term_handle term;
} io_term;

static io_term stdout, stdwarn, stderr, stdpanic;

static io_term* const STDIO_OUTPUTS[IO_COUNT] = {
    [IO_STDOUT]   = &stdout,
    [IO_STDWARN]  = &stdwarn,
    [IO_STDERR]   = &stderr,
    [IO_STDPANIC] = &stdpanic,
};

static void handle_notify_activation(io_term* io, bool finished_output);

static void io_term_new(io_term* new, const char* device_name, term_out output)
{
    if (device_name) {
        size_t len  = strlen(device_name) + 1;
        char*  name = kmalloc(len);
        strcopy(name, device_name, len);

        device_name = name;
    }

    new->device_name    = device_name;
    new->notify_enabled = false;
    new->lock           = SPINLOCK_INIT;
    term_new(&new->term, output);
}


void io_init()
{
    for (size_t i = 0; i < ARRAY_LEN(STDIO_OUTPUTS); i++) {
        io_term_new(STDIO_OUTPUTS[i], NULL, STDIO_PUTC[i]);
    }
}


// void io_flush(io_out io)
// {
//     term_flush(STDIO_OUTPUTS[io]);
// }

static void handle_tx_ready_notification(void* ctx)
{
    io_term* io              = ctx;
    bool     finished_output = term_notify_ready(&io->term);

    handle_notify_activation(io, finished_output);
}

static void handle_notify_activation(io_term* io, bool finished_output)
{
    const device_t* output_dev = (io->device_name == NULL)
                                     ? device_get_primary(DEVICE_CLASS_SERIAL)
                                     : device_get_by_name(
                                           DEVICE_CLASS_SERIAL,
                                           io->device_name);

    driver_handle_t     handle = device_get_driver_handle(output_dev);
    const serial_ops_t* ops    = ((serial_ops_t*)output_dev->driver_ops);


    irq_spinlocked(&io->lock)
    {
        bool notify_enabled = io->notify_enabled;

        if (finished_output == !notify_enabled)
            return;

        int32_t res;
        if (finished_output) {
            // irqs enabled but message fully sent, disable notify
            res                = ops->irq_notify_tx(handle, 0, NULL, NULL);
            io->notify_enabled = false;
        }
        else {
            // irqs disabled but the device couldn't finish the transmision so
            // we must defer the output sending by waiting for the "ready for
            // tx" irq
            res = ops->irq_notify_tx(
                handle,
                1,
                handle_tx_ready_notification,
                io);
            io->notify_enabled = true;
        }
        ASSERT(res >= 0);
    }
}

void fkprint(io_out io, const char* s)
{
    DEBUG_ASSERT(io >= 0 && io < IO_COUNT);

    bool finished_output = term_prints(&STDIO_OUTPUTS[io]->term, s);

    handle_notify_activation(STDIO_OUTPUTS[io], finished_output);
}

void fkprintf(io_out io, const char* s, ...)
{
    va_list va;
    va_start(va, s);

    DEBUG_ASSERT(io >= 0 && io < IO_COUNT);

    bool finished_output = term_printf(&STDIO_OUTPUTS[io]->term, s, va);

    handle_notify_activation(STDIO_OUTPUTS[io], finished_output);

    va_end(va);
}
