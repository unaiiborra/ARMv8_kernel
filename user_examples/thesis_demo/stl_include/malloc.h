#pragma once

#include <stddef.h>

void* alloc(size_t bytes);
void* zalloc(size_t bytes);
void  free(void* ptr);
