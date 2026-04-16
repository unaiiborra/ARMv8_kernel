#include "stdio.h"

#include <stddef.h>

#include "stdlib.h"
#include "syscall.h"


void print(const char* s)
{
    const char* end = s;

    while (*end)
        end++;

    size_t size = end - s;

    sysc_print_res result = syscall_print(s, size);

    if (result != SYSC_PRINT_OK)
        exit(1);
}


// TODO: void printf(const char* s, ...);
