#include <kernel/exception/irq.h>
#include <lib/stdattribute.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <target/imx8mp.h>

#include "kernel/devices/device.h"
#include "kernel/devices/drivers.h"
#include "kernel/io/stdio.h"
#include "kernel/panic.h"
#include "lib/branch.h"
#include "lib/mem.h"
#include "lib/string.h"

#ifndef MAX_IRQS
#    pragma message("TODO: MAX_IRQS should be defined globally")
#    define MAX_IRQS IMX8MP_IRQ_SIZE
#endif


typedef enum {
    IRQ_UNREGISTERED,
    IRQ_REGISTERING,
    IRQ_REGISTERED,
} irq_register_status;

typedef enum {
    STD_HANDLER,
    DRIVER_HANDLER,
} irq_handler_type_t;

typedef struct {
    irq_handler_type_t handler_type;
    atomic_int         register_status;
    irq_handler_t      handler;
    void*              ctx;
} irq_entry_t;

typedef struct {
    char*          driver_name;
    device_class_t driver_class;
} driver_ctx_t;


static irq_entry_t irq_table[MAX_IRQS];


[[gnu::always_inline]] static inline const device_t* irq_dev()
{
    return device_get_primary(DEVICE_CLASS_IRQ_CTRL);
}

[[gnu::always_inline]] static inline const irq_ctrl_ops_t*
irq_ops(const device_t* dev)
{
    return (const irq_ctrl_ops_t*)dev->driver_ops;
}

[[gnu::always_inline]] static inline driver_handle_t
irq_driver_handle(const device_t* dev)
{
    return device_get_driver_handle(dev);
}



static void irq_ctrl_config(
    uint32_t               irq_id,
    irq_ctrl_ops_trigger_t trigger,
    cpuid_t                target_cpu,
    uint8_t                priority)
{
    const device_t*       dev = irq_dev();
    const irq_ctrl_ops_t* ops = irq_ops(dev);
    driver_handle_t       h   = irq_driver_handle(dev);

    dbgT(int32_t) op_res[4];

    op_res[0] = ops->irq_set_priority(h, irq_id, priority);
    op_res[1] = ops->irq_set_trigger(h, irq_id, trigger);
    op_res[2] = ops->irq_set_target(h, irq_id, target_cpu);
    op_res[3] = ops->irq_enable(h, irq_id);

#ifdef DEBUG
    for (size_t i = 0; i < 4; i++)
        DEBUG_ASSERT(op_res[i] >= 0);
#endif
}


static void irq_register_type(
    irq_handler_type_t type,
    uint32_t           irq_id,
    void*              handler,
    void*              ctx)
{
    ASSERT(irq_id < MAX_IRQS);
    ASSERT(handler != NULL);

    int  expected = IRQ_UNREGISTERED;
    bool ok       = atomic_compare_exchange_strong(
        &irq_table[irq_id].register_status,
        &expected,
        IRQ_REGISTERING);
    ASSERT(ok, "irq_register_driver: double register");

    irq_table[irq_id].handler.any  = handler;
    irq_table[irq_id].ctx          = ctx;
    irq_table[irq_id].handler_type = type;

    atomic_store_explicit(
        &irq_table[irq_id].register_status,
        IRQ_REGISTERED,
        memory_order_release);
}


void irq_register(
    uint32_t               irq_id,
    irq_std_handler_t      handler,
    void*                  ctx,
    irq_ctrl_ops_trigger_t trigger,
    cpuid_t                target_cpu,
    uint8_t                priority)
{
    irq_register_type(STD_HANDLER, irq_id, handler, ctx);
    irq_ctrl_config(irq_id, trigger, target_cpu, priority);
}


void irq_register_driver(
    uint32_t       irq_id,
    const char*    driver_name,
    device_class_t driver_class,
    void (*driver_irq_handler)(driver_handle_t handle),
    irq_ctrl_ops_trigger_t trigger,
    cpuid_t                target_cpu,
    uint8_t                priority)
{
    driver_ctx_t* ctx = kmalloc(sizeof(driver_ctx_t));

    size_t size       = strlen(driver_name) + 1;
    ctx->driver_name  = kmalloc(size);
    ctx->driver_class = driver_class;
    strcopy(ctx->driver_name, driver_name, size);

    irq_register_type(DRIVER_HANDLER, irq_id, driver_irq_handler, ctx);
    irq_ctrl_config(irq_id, trigger, target_cpu, priority);
}


void irq_unregister(uint32_t irq_id)
{
    ASSERT(irq_id < MAX_IRQS);

    int  expected = IRQ_REGISTERED;
    bool ok       = atomic_compare_exchange_strong(
        &irq_table[irq_id].register_status,
        &expected,
        IRQ_REGISTERING);
    ASSERT(ok);

    const device_t*       dev = irq_dev();
    const irq_ctrl_ops_t* ops = irq_ops(dev);
    driver_handle_t       h   = irq_driver_handle(dev);


    dbgT(int32_t) op_res = ops->irq_disable(h, irq_id);
    DEBUG_ASSERT(op_res >= 0);


    if (irq_table[irq_id].handler_type == DRIVER_HANDLER) {
        driver_ctx_t* ctx = irq_table[irq_id].ctx;
        kfree(ctx->driver_name);
        kfree(ctx);
    }

    irq_table[irq_id].handler.any = NULL;
    irq_table[irq_id].ctx         = NULL;

    atomic_store_explicit(
        &irq_table[irq_id].register_status,
        IRQ_UNREGISTERED,
        memory_order_release);
}


void irq_dispatch()
{
    dbgT(int32_t) op_res;
    const device_t*       dev    = irq_dev();
    const irq_ctrl_ops_t* ops    = irq_ops(dev);
    driver_handle_t       handle = irq_driver_handle(dev);

    uint32_t irq = ops->irq_ack(handle);

    irq_entry_t* entry = &irq_table[irq];

    irq_register_status status =
        atomic_load_explicit(&entry->register_status, memory_order_acquire);

    if (unlikely(status != IRQ_REGISTERED)) {
        dbg_printf(DEBUG_TRACE, "irqid %d arrived but not registered!", irq);

        op_res = ops->irq_eoi(handle, irq);
        DEBUG_ASSERT(op_res >= 0);

        return;
    }

    DEBUG_ASSERT(entry->handler.any != NULL);

    switch (entry->handler_type) {
        case STD_HANDLER: {
            entry->handler.std_handler(entry->ctx);
            break;
        }
        case DRIVER_HANDLER: {
            driver_ctx_t*   dctx = entry->ctx;
            const device_t* drv_dev =
                device_get_by_name(dctx->driver_class, dctx->driver_name);
            ASSERT(drv_dev != NULL, "irq_dispatch: driver not found");

            driver_handle_t drv_handle = device_get_driver_handle(drv_dev);

            entry->handler.driver_handler(drv_handle);

            break;
        }
    }

    op_res = ops->irq_eoi(handle, irq);
    DEBUG_ASSERT(op_res >= 0);
}