#include "sysc_print.h"

#include <kernel/io/stdio.h>
#include <kernel/lib/kvec.h>
#include <kernel/process/thread.h>
#include <kernel/syscall.h>
#include <lib/mem.h>
#include <lib/stdint.h>


int64 sysc64_print(const uint64 args[6])
{
    const thread* th = thread_get_current();

    proc_map_info pmap;
    v_uintptr pt_buf = args[0];

    bool mapped = proc_get_usrmap_info(th->th_ctx->owner, pt_buf, &pmap);
    if (!mapped || !pmap.read || pmap.execute)
        return SYSC_PRINT_INVALID_BUF;


    const size_t N = args[1];
    char* buf = kmalloc(N + 1);
    size_t offset = pt_buf - pmap.usr_va;

    memcpy(buf, (char*)(pmap.knl_va + offset), N);
    buf[N] = '\0';


    fkprint(IO_STDOUT, buf);


    return SYSC_PRINT_OK;
}
