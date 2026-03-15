#pragma once

#include <stdint.h>


#define SYSC64_SYSCNUM_REG 8
#define SYSC64_RETURN_REG 0


#define SYSC64_RES_UNKNOWN 0xDEADBEEFDEADBEEF


typedef int64_t (*syscall_handler)(const uint64_t args[6]);


int64_t syscall64_print(const uint64_t args[6]);