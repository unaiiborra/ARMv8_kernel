#pragma once

#include <kernel/mm/vmalloc.h>
#include "../init/mem_regions/early_kalloc.h"


void vmalloc_init();
v_uintptr vmalloc_update_memregs(const early_memreg *mregs, size_t n);


