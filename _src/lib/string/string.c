#include <kernel/panic.h>
#include <lib/string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
void strcopy(char* dst, const char* src, uint64_t max_size)
{
    for (uint64_t i = 0; i < max_size - 1; i++) {
        dst[i] = src[i];

        if (src[i] == '\0')
            return;
    }

    dst[max_size - 1] = '\0';
}


bool strcmp(const char* a, const char* b)
{
    if (a == b)
        return true;

    if (!a || !b) {
        if (!a && !b)
            return true;
        else
            return false;
    }

    size_t i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i])
            return false;
        i++;
    }

    if (!a[i] && !b[i])
        return true;

    return false;
}
