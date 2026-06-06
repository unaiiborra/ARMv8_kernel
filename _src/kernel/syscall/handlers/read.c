// #include <stdint.h>

// #include "../sysc_handlers.h"
// #include "kernel/mm/uregion.h"
// #include "kernel/scheduler.h"
// #include "kernel/task.h"
// #include "kernel/vfs.h"
// #include "lib/lock.h"



// int64_t syscall64_read(
//     sysarg_t                  fd,
//     sysarg_t                  buf,
//     sysarg_t                  count,
//     [[maybe_unused]] sysarg_t a3,
//     [[maybe_unused]] sysarg_t a4,
//     [[maybe_unused]] sysarg_t a5)
// {
//     task_t* task = get_current_thread()->owner;

//     if (fd > INT32_MAX)
//         return VFS_ERR_BADF;

//     if (count == 0) {
//         return VFS_ERR_INVAL;
//     }

//     spinlocked(&task->lock)
//     {
//         uregion_access_e access = uregions_check_access(
//             task,
//             buf,
//             count,
//             UREGION_F_WRITE,
//             UREGION_F_EXEC,
//             true);

//         if (access != UREGION_ACCESS_OK)
//             return VFS_ERR_INVAL;

//         uint8_t      kbuf[count];
//         vfs_result_t vfs_res = vfs_read(&task->files, fd, kbuf, count);

//         if (vfs_res != VFS_OK)
//             return vfs_res;

        
//     }
// }
