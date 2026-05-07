#include <kernel/scheduler.h>
#include <lib/ansi.h>
#include <stddef.h>
#include <stdint.h>

#include "../sysc_handlers.h"
#include "kernel/io/stdio.h"
#include "kernel/mm.h"
#include "kernel/mm/uregion.h"
#include "kernel/panic.h"
#include "kernel/task.h"
#include "lib/stdattribute.h"


#define BUF_PTR_ARG  0
#define BUF_SIZE_ARG 1

typedef enum {
    SYSC_PRINT_OK          = 0,
    SYSC_PRINT_INVALID_BUF = -1,
} sysc_print_res;

int64_t syscall64_print(
    sysarg_t                  buf_pt,
    sysarg_t                  buf_sz,
    [[maybe_unused]] sysarg_t a2,
    [[maybe_unused]] sysarg_t a3,
    [[maybe_unused]] sysarg_t a4,
    [[maybe_unused]] sysarg_t a5)
{
    uregion_t* region;
    if (!uregion_is_committed(
            get_current_thread()->owner,
            buf_pt,
            buf_sz,
            &region)) {
        dbg_sysc_print(SYSC_PRINT, "SYSC_PRINT_INVALID_BUF");

        return SYSC_PRINT_INVALID_BUF;
    }

    uintptr_t kva;
    dbgT(bool) valid = uregion_get_knl_access(region, buf_pt, &kva);
    DEBUG_ASSERT(valid);


    char* cpy   = kmalloc(buf_sz + 1); // copy to avoid reading out of BUF_SIZE
    cpy[buf_sz] = '\0';

    memcpy(cpy, (void*)kva, buf_sz);

#if DEBUG < DEBUG_TRACE
    fkprint(IO_STDOUT, cpy);
#else
    dbg_sysc_print(
        SYSC_PRINT,
        "\"" ANSI_RESET "%s" DEBUG_ANSI_FG_COLOR "\"",
        cpy);
#endif


    kfree(cpy);

    return SYSC_PRINT_OK;
}
