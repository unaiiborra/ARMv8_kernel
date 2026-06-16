// `#include <kernel/vfs.h>
// #include <stdint.h>

// #include "../sysc_handlers.h"
// #include "kernel/mm.h"
// #include "kernel/mm/uregion.h"
// #include "kernel/scheduler.h"

// #define MAX_STRLEN 4096

// int64_t syscall64_openat(
//     sysarg_t        dirfd,
//     sysarg_t        pathname,
//     sysarg_t        flags,
//     unused_sysarg_t a3,
//     unused_sysarg_t a4,
//     unused_sysarg_t a5)
// {
//     scoped_kfree_t path_buf = kmalloc(MAX_STRLEN);
//     char*          path     = path_buf;

//     uregion_access_e uaccess = ustrncpy(
//         get_current_thread()->owner,
//         path_buf,
//         (void*)pathname,
//         UREGION_F_READ,
//         UREGION_F_EXEC,
//         MAX_STRLEN);

//     if (uaccess == UREGION_ACCESS_TRUNCATED) {
//         return VFS;
//     }
//     else if (uaccess != UREGION_ACCESS_OK)
//         return VFS_ERR_INVAL;
// }
