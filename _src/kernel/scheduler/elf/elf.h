#pragma once

#include <kernel/scheduler/process.h>

typedef enum {
    ELF_LOAD_OK,
    ELF_LOAD_BAD_MAGIC,
    ELF_LOAD_BAD_HDR,
} elf_load_result;

elf_load_result elf_load(void* elf, size_t size, proc* usr_proc);