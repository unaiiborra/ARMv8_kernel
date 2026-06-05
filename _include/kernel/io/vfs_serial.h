#pragma once

#include <kernel/devices/device.h>
#include <kernel/vfs.h>

// fd 0 1 and 2 must be free for the provided table. The provided device must be
// already initialized and its irq enabled.
void vfs_serial_bind_stdio(fd_table_t* table, uint64_t device_uid);
