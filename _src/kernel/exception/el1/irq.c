/*#include <drivers/uart/uart.h>
#include <kernel/devices/drivers.h>
#include <kernel/exception/interrupts.h>
#include <kernel/init.h>
#include <kernel/panic.h>
#include <lib/stdmacros.h>
#include <stddef.h>

#include "drivers/arm_generic_timer/arm_generic_timer.h"
#include "drivers/interrupts/gicv3/gicv3.h"
#include "drivers/tmu/tmu.h"
#include "imx8mp.h"
#include "kernel/devices/device.h"
#include "kernel/io/stdio.h"

typedef void (*driver_irq_handler)(const driver_handle* h);

typedef struct {
    driver_irq_handler   handler;
    const driver_handle* h;
} kernel_irq_handler;

static kernel_irq_handler KERNEL_IRQ_HANDLER_TABLE_[IMX8MP_IRQ_SIZE];

static void unhandled_irq_(const driver_handle*)
{
    PANIC("UNHANDLED IRQ");
}

static inline kernel_irq_handler
build_handler_(driver_irq_handler handler, const driver_handle* h)
{
    return (kernel_irq_handler) {
        .handler = handler,
        .h       = h,
    };
}


static void handle_uart_test(const driver_handle* h)
{
    uart_handle_irq(h);
    io_flush(IO_STDOUT);
}


static void init_irq_handler_table_()
{
    for (size_t i = 0; i < IMX8MP_IRQ_SIZE; i++)
        KERNEL_IRQ_HANDLER_TABLE_[i] =
            (kernel_irq_handler) {unhandled_irq_, NULL};

    KERNEL_IRQ_HANDLER_TABLE_[IMX8MP_IRQ_UART1] =
        build_handler_(uart_handle_irq, &UART1_DRIVER);
    KERNEL_IRQ_HANDLER_TABLE_[IMX8MP_IRQ_UART2] =
        build_handler_(handle_uart_test, &UART2_DRIVER);
    KERNEL_IRQ_HANDLER_TABLE_[IMX8MP_IRQ_UART3] =
        build_handler_(uart_handle_irq, &UART3_DRIVER);
    KERNEL_IRQ_HANDLER_TABLE_[IMX8MP_IRQ_UART4] =
        build_handler_(uart_handle_irq, &UART4_DRIVER);

    KERNEL_IRQ_HANDLER_TABLE_[IMX8MP_IRQ_ANAMIX_TEMP] =
        build_handler_(TMU_handle_irq, &TMU_DRIVER);

    // TODO: 0..31 irq enum
    KERNEL_IRQ_HANDLER_TABLE_[27] =
        build_handler_(AGT_handle_irq, &AGT0_DRIVER);
}

KERNEL_INITCALL(init_irq_handler_table_);


void exception_handler_irq()
{
    irq_id irq = GICV3_get_intid_el1();



    KERNEL_IRQ_HANDLER_TABLE_[irq.n].handler(
        KERNEL_IRQ_HANDLER_TABLE_[irq.n].h);

    GICV3_ack_intid_el1(irq);
}
*/