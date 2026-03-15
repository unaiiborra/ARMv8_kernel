#include <kernel/scheduler.h>
#include <stddef.h>
#include <stdint.h>

#include "../sysc_handlers.h"
#include "kernel/io/stdio.h"
#include "kernel/mm.h"
#include "kernel/mm/umalloc.h"


#define BUF_PTR_ARG 0
#define BUF_SIZE_ARG 1


typedef enum {
    SYSC_PRINT_OK = 0,
    SYSC_PRINT_INVALID_BUF = -1,
} sysc_print_results;


int64_t syscall64_print(const uint64_t args[6])
{
    const uintptr_t BUF = args[BUF_PTR_ARG];
    const size_t BUF_SIZE = (size_t)args[BUF_SIZE_ARG];


    usr_region* region;
    if (!uregion_is_assigned(
            get_current_thread()->task.utask,
            (uintptr_t)BUF,
            BUF_SIZE,
            &region))
        return SYSC_PRINT_INVALID_BUF;


    size_t offset = BUF - region->any.usr_start;
    const char* kva = (char*)(region->any.knl_start + offset);

    char* cpy = kmalloc(BUF_SIZE + 1); // copy to avoid reading out of BUF_SIZE
    cpy[BUF_SIZE] = '\0';

    memcpy(cpy, kva, BUF_SIZE);

    fkprint(IO_STDOUT, cpy);

    kfree(cpy);

    return SYSC_PRINT_OK;
}
