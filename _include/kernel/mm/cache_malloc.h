#pragma once

#include <lib/math.h>
#include <stddef.h>


typedef enum {
    CACHE_8    = 8,
    CACHE_16   = 16,
    CACHE_32   = 32,
    CACHE_64   = 64,
    CACHE_128  = 128,
    CACHE_256  = 256,
    CACHE_512  = 512,
    CACHE_1024 = 1024,
    CACHE_2048 = 2048,
} cache_malloc_size;


void* cache_malloc(cache_malloc_size s);
void  cache_free(void* ptr);
