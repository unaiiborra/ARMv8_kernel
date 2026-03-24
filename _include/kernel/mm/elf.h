#pragma once

#include <kernel/scheduler.h>
#include <stdint.h>

typedef enum {
    ELF_LOAD_OK,
    ELF_LOAD_BAD_MAGIC,
    ELF_LOAD_BAD_HDR,
} elf_load_result;

elf_load_result
elf_load(task* proc, void* elf, size_t size, uintptr_t* out_entry);