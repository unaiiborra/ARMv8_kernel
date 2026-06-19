#include <kernel/mm/uregion.h>
#include <kernel/vfs.h>
#include <lib/lock.h>

#include "../sysc_handlers.h"
#include "kernel/mm.h"

int64_t syscall64_write(
    sysarg_t        fd,
    sysarg_t        buf,
    sysarg_t        count,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5)
{
    if ((int64_t)fd < 0 || fd > INT32_MAX)
        return VFS_ERR_BADF;

    if (count == 0 || count > VFS_MAX_WRITE_SIZE)
        return VFS_ERR_INVAL;

    task_t*        task      = get_current_thread()->owner;
    scoped_kfree_t write_buf = kzalloc(count);

    uregion_access_e uaccess;
    spinlocked_irqsave(&task->memory_lock)
    {
        uaccess = umemcpy(
            task,
            write_buf,
            (void*)buf,
            count,
            UREGION_F_READ,
            0,
            false,
            UMEMCPY_USR_TO_KNL);
    }

    if (uaccess != UREGION_ACCESS_OK)
        return VFS_ERR_INVAL;

    return vfs_write(&task->files, fd, write_buf, count);
}
