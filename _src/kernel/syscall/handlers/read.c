#include <stdint.h>

#include "../sysc_handlers.h"
#include "kernel/mm.h"
#include "kernel/mm/uregion.h"
#include "kernel/scheduler.h"
#include "kernel/task.h"
#include "kernel/vfs.h"
#include "lib/lock.h"

int64_t syscall64_read(
    sysarg_t        fd,
    sysarg_t        buf,
    sysarg_t        count,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5)
{
    if ((int64_t)fd < 0 || fd > INT32_MAX)
        return VFS_ERR_BADF;

    if (count == 0 || count > VFS_MAX_READ_SIZE)
        return VFS_ERR_INVAL;

    task_t*        task     = get_current_thread()->owner;
    scoped_kfree_t read_buf = kzalloc(count);

    vfs_result_t vfs_res = vfs_read(&task->files, fd, read_buf, count);

    if (vfs_res < 0)
        return vfs_res;

    uregion_access_e uaccess;

    irqflags_t irqflags = spinlock_acquire_irqsave(&task->lock);
    {
        uaccess = umemcpy(
            task,
            (void*)buf,
            read_buf,
            vfs_res,
            UREGION_F_WRITE,
            UREGION_F_EXEC,
            true,
            UMEMCPY_KNL_TO_USR);
    }
    spinlock_release_irqrestore(&task->lock, irqflags);

    if (uaccess != UREGION_ACCESS_OK)
        return VFS_ERR_INVAL;

    return vfs_res;
}
