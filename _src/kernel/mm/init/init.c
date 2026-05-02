
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/stdattribute.h>

#include "../init/mem_regions/early_kalloc.h"
#include "../malloc/cache_malloc/cache_malloc.h"
#include "../malloc/internal/reserve_malloc.h"
#include "../malloc/raw_kmalloc/raw_kmalloc.h"
#include "../mm_info.h"
#include "../mm_mmu/mm_mmu.h"
#include "../phys/page_allocator.h"
#include "../reloc/reloc.h"
#include "../virt/vmalloc.h"
#include "identity_mapping.h"


safe_early void mm_early_init()
{
    mm_info_init();

    // init early kalloc
    early_kalloc_init();

    // init MM_MMU_UNMAPPED_LO
    mm_mmu_early_init();

    // init identity mapping
    early_identity_mapping();

    // page allocator
    page_allocator_init();

    // virtual allocator
    vmalloc_init();

    // reserve allocator
    reserve_malloc_init();


    early_memreg* mregs;
    size_t        n;
    early_kalloc_get_memregs(&mregs, &n);

    page_allocator_update_memregs(mregs, n);
    vmalloc_update_memregs(mregs, n);


    // early_reloc_cfg_end() returns to the kernel_entry() with the kernel
    // relocated and the sp resetted

    mm_reloc(as_kva((void*)early_reloc_cfg_end));
}


void mm_init()
{
    raw_kmalloc_init();

    cache_malloc_init();
}
