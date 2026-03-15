#pragma once

#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    _Alignas(16) puintptr_t addr;
    _Alignas(16) size_t pages;
    _Alignas(16) bool free;
    _Alignas(16) const char* tag;
    _Alignas(16) bool permanent;
    _Alignas(16) bool device_memory;
} early_memreg;


void early_kalloc_init();


/// Allocates a kernel region and saves the memory regions it allocated for
/// later initialization of other allocators
pv_ptr
early_kalloc(size_t bytes, const char* tag, bool permanent, bool device_memory);


/// Returns the pointer to the memregions so later stage allocators like the page
/// allocators can update their structs and be coherent with the kernel memory
/// regions. The first time called it reallocates all the structure to the end
/// of the last allocated block and autoassigns itself as a non permanent block.
/// It is the other allocators job to free the early_kalloc block.
void early_kalloc_get_memregs(early_memreg** memregs, size_t* mreg_struct_count);


void early_kalloc_debug();
