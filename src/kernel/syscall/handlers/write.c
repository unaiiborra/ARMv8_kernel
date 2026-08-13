#include <kernel/mm/uregion.h>
#include <kernel/vfs.h>
#include <lib/lock.h>

#include "../sysc_handlers.h"
#include "arm/exceptions/exceptions.h"
#include "kernel/mm.h"
#include "lib/stdattribute.h"

int64_t syscall64_write(
    sysarg_t        fd,
    sysarg_t        buf,
    sysarg_t        count,
    unused_sysarg_t a3,
    unused_sysarg_t a4,
    unused_sysarg_t a5)
{
    vfs_result_t       result    = VFS_OK;
    maybe_unused char* message   = NULL;
    scoped_kfree_t     write_buf = NULL;

    if ((int64_t)fd < 0 || fd > INT32_MAX) {
        result = VFS_ERR_BADF;
        goto end;
    }

    if (count == 0 || count > VFS_MAX_WRITE_SIZE) {
        result = VFS_ERR_INVAL;
        goto end;
    }

    task_t* task = get_current_thread()->owner;
    write_buf    = kzalloc(count + 1);
    message      = write_buf;

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

    if (uaccess != UREGION_ACCESS_OK) {
        result = VFS_ERR_INVAL;
        goto end;
    }

    result = vfs_write(&task->files, fd, write_buf, count);

end:
    switch (result) {
        case VFS_ERR_BADF:
            dbg_sysc_print(SYSC_WRITE, "VFS_ERR_BADF");
            break;
        case VFS_ERR_NODEV:
            dbg_sysc_print(SYSC_WRITE, "VFS_ERR_NODEV");
            break;
        case VFS_ERR_NOSUP:
            dbg_sysc_print(SYSC_WRITE, "VFS_ERR_NOSUP");
            break;
        case VFS_ERR_INVAL:
            dbg_sysc_print(SYSC_WRITE, "VFS_ERR_INVAL");
            break;
        default:
#if DEBUG == DEBUG_TRACE
            if (result >= 0)
                dbg_sysc_print(SYSC_WRITE, "VFS_OK (%s)", message);
            else
                dbg_sysc_print(SYSC_WRITE, "(%s)", message);
#endif
            break;
    }

    return result;
}
