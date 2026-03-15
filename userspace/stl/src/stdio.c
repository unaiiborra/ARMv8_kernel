#include <stddef.h>
#include <stdio.h>
#include <syscall.h>


void print(const char* s)
{
    const char* end = s;

    while (*end)
        end++;

    size_t size = end - s;

    sysc_print_results result = syscall_print(s, size);

    if (result != SYSC_PRINT_OK) {
        // TODO: exit()
    }
}


// TODO: void printf(const char* s, ...);
