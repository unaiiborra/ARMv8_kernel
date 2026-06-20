#include <kernel/io/vfs_serial.h>
#include <kernel/vfs.h>
#include <stddef.h>
#include <stdint.h>

#include "kernel/devices/device.h"
#include "kernel/devices/driver_ops/serial.h"
#include "kernel/io/term.h"
#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/lock.h"
#include "lib/stdattribute.h"

typedef struct {
    uint64_t    dev_uid;
    term_handle term;
    bool        notify_enabled;
    spinlock_t  lock; // irq spinlocks notify_enabled
} vfs_serial_data_t;

static constexpr file_descriptor_t FD_STDIN  = 0;
static constexpr file_descriptor_t FD_STDOUT = 1;
static constexpr file_descriptor_t FD_STDERR = 2;

static vfs_result_t stdin_read(void* ctx, uint8_t* buf, uint32_t len);
static vfs_result_t stdoutput_write(void* ctx, const uint8_t* buf, uint32_t len);
static void handle_term_notify(vfs_serial_data_t* device_data, bool finished);

static const file_operations_t stdin_ops = {
    .read  = stdin_read,
    .write = NULL,
    .open  = NULL,
    .close = NULL,
    .mmap  = NULL,
};

static const file_operations_t stdout_stderr_ops = {
    .read  = NULL,
    .write = stdoutput_write,
    .open  = NULL,
    .close = NULL,
    .mmap  = NULL,
};

static int32_t stdout_stderr_putc(const char c, void* ctx)
{
    uint64_t device_uid = (uint64_t)ctx;

    const device_t* serial = device_get_by_uid(DEVICE_CLASS_SERIAL, device_uid);
    ASSERT(serial);
    DEBUG_ASSERT(serial->class_id == DEVICE_CLASS_SERIAL);

    const serial_ops_t* ops     = get_serial_ops(serial);
    driver_handle_t     dhandle = device_get_driver_handle(serial);

    return ops->putc(dhandle, c);
}


// returns -1, never takes data because the input must always be stored at the
// term dta structures until read with term_remove_head
static int32_t stdin_putc(const char c, void* ctx)
{
    (void)c, (void)ctx;

    return -1;
}

// stdin read uses the term in the opposite way from stdout/err. Printing
// (receiving data) makes the buffer grow, which then is removed by calling
// term_remove_head. No lock needed as term has already a lock and there is only
// one data provider (the serial driver)
static vfs_result_t stdin_read(void* ctx, uint8_t* buf, uint32_t len)
{
    if (unlikely(len == 0 || !buf || !ctx))
        return VFS_ERR_INVAL;

    vfs_serial_data_t* device_data = ctx;

    // return removed count
    return term_remove_head(&device_data->term, (char*)buf, len);
}

static void on_rx_ready(void* ctx)
{
    vfs_serial_data_t* device_data = ctx;

    const device_t* serial = device_get_by_uid(
        DEVICE_CLASS_SERIAL,
        device_data->dev_uid);

    ASSERT(serial != NULL);
    DEBUG_ASSERT(serial->class_id == DEVICE_CLASS_SERIAL);

    const serial_ops_t* serial_ops = get_serial_ops(serial);
    driver_handle_t     dhandle    = device_get_driver_handle(serial);

    while (true) {
        int32_t c = serial_ops->getc(dhandle);

        if ((c & ~0xff) != 0) // c < 0 || c > UINT8_MAX
            break;

        term_printc(&device_data->term, c);
    }
}

static void on_tx_ready(void* ctx)
{
    vfs_serial_data_t* device_data     = ctx;
    bool               finished_output = term_notify_ready(&device_data->term);

    handle_term_notify(device_data, finished_output);
}

static void handle_term_notify(vfs_serial_data_t* device_data, bool finished)
{
    const device_t* serial = device_get_by_uid(
        DEVICE_CLASS_SERIAL,
        device_data->dev_uid);

    ASSERT(serial);

    const serial_ops_t* ops    = get_serial_ops(serial);
    driver_handle_t     handle = device_get_driver_handle(serial);

    spinlocked_irqsave(&device_data->lock)
    {
        bool notify_enabled = device_data->notify_enabled;

        // already in the correct state
        if (finished == !notify_enabled)
            return;

        maybe_unused int32_t res;
        if (finished) {
            // irqs enabled but message fully sent, disable notify
            res = ops->irq_notify_tx(handle, 0, NULL, NULL);
            device_data->notify_enabled = false;
        }
        else {
            // irqs disabled but the device couldn't finish the transmision so
            // we must defer the output sending by waiting for the "ready for
            // tx" irq
            res = ops->irq_notify_tx(handle, 1, on_tx_ready, device_data);

            device_data->notify_enabled = true;
        }

        DEBUG_ASSERT(res >= 0);
    }
}

static vfs_result_t stdoutput_write(void* ctx, const uint8_t* buf, uint32_t len)
{
    if (unlikely(len == 0 || !buf || !ctx))
        return VFS_ERR_INVAL;

    vfs_serial_data_t* device_data = ctx;

    bool finished = term_print_slice(&device_data->term, (const char*)buf, len);

    handle_term_notify(device_data, finished);

    return (vfs_result_t)len;
}

void vfs_serial_bind_stdio(fd_table_t* table, uint64_t device_uid)
{
    const device_t* serial = device_get_by_uid(DEVICE_CLASS_SERIAL, device_uid);
    ASSERT(serial != NULL);
    DEBUG_ASSERT(serial->class_id == DEVICE_CLASS_SERIAL);

    // stdout y stderr share lock and buffer
    vfs_serial_data_t* out_data = kmalloc(sizeof(vfs_serial_data_t));
    vfs_serial_data_t* in_data  = kmalloc(sizeof(vfs_serial_data_t));

    *out_data = (vfs_serial_data_t) {
        .dev_uid        = device_uid,
        .notify_enabled = false,
        .lock           = SPINLOCK_INIT,
    };
    term_new(&out_data->term, stdout_stderr_putc, (void*)device_uid);

    *in_data = (vfs_serial_data_t) {
        .dev_uid        = device_uid,
        .notify_enabled = false,
        .lock           = SPINLOCK_INIT,
    };
    term_new(&in_data->term, stdin_putc, (void*)device_uid);

    irqflags_t irqflags = irqsave();

    vfs_result_t res;
    res = vfs_bind(table, FD_STDIN, &stdin_ops, in_data);
    ASSERT(res == VFS_OK);
    res = vfs_bind(table, FD_STDOUT, &stdout_stderr_ops, out_data);
    ASSERT(res == VFS_OK);
    res = vfs_bind(table, FD_STDERR, &stdout_stderr_ops, out_data);
    ASSERT(res == VFS_OK);

    // register irqs for serial rx -> stdin
    const serial_ops_t* serial_ops = get_serial_ops(serial);
    driver_handle_t     dhandle    = device_get_driver_handle(serial);

    int32_t rx_res = serial_ops->irq_notify_rx(dhandle, 1, on_rx_ready, in_data);
    ASSERT(rx_res >= 0);

    irqrestore(irqflags);
}
