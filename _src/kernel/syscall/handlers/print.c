#include <kernel/scheduler.h>
#include <stddef.h>
#include <stdint.h>

#include "../sysc_handlers.h"
#include "kernel/io/stdio.h"
#include "kernel/mm.h"
#include "kernel/mm/umalloc.h"
#include "kernel/panic.h"
#include "kernel/task.h"


#define BUF_PTR_ARG  0
#define BUF_SIZE_ARG 1


typedef enum {
    SYSC_PRINT_OK          = 0,
    SYSC_PRINT_INVALID_BUF = -1,
} sysc_print_results;


int64_t syscall64_print(
    sysarg_t buf_pt,
    sysarg_t buf_sz,
    sysarg_t a2,
    sysarg_t a3,
    sysarg_t a4,
    sysarg_t a5)
{
    (void)a2, (void)a3, (void)a4, (void)a5;

    task_region* region;
    if (!uregion_is_assigned(
            get_current_thread()->owner,
            buf_pt,
            buf_sz,
            &region))
        return SYSC_PRINT_INVALID_BUF;


    uintptr_t kva = task_region_translate(region, buf_pt, REG_TO_KNL);

#ifdef DEBUG
    if (kva == TASK_REGION_TRANSLATE_ERR)
        PANIC("should be handled by uregion_is_assigned");
#endif

    char* cpy   = kmalloc(buf_sz + 1); // copy to avoid reading out of BUF_SIZE
    cpy[buf_sz] = '\0';

    memcpy(cpy, (void*)kva, buf_sz);

    fkprint(IO_STDOUT, cpy);

    kfree(cpy);

    return SYSC_PRINT_OK;
}
