#pragma once

#if __SIZEOF_POINTER__ == 4
typedef int intptr;
typedef int int64_t;
typedef unsigned int uintptr_t;
typedef unsigned int size_t;
#elif __SIZEOF_POINTER__ == 8
#    if __SIZEOF_LONG__ == 8
typedef long intptr;
typedef long int64_t;

typedef unsigned long uintptr_t;
typedef unsigned long size_t;
#    else
typedef long long intptr;
typedef unsigned long long uintptr_t;
#    endif

#else
#    error "Unsupported pointer size"
#endif

#define SIZE_MAX ((size_t)-1)