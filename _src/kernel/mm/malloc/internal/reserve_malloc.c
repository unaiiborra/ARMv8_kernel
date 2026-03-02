#include "reserve_malloc.h"

#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <lib/stdint.h>
#include <lib/string.h>

#include "../../init/mem_regions/early_kalloc.h"
#include "../../mm_info.h"
#include "../../phys/page_allocator.h"
#include "../../virt/vmalloc.h"

static const char *RESERVE_MALLOC_TAG = "reserved page";


#define RESERVE_MALLOC_SIZE    BITFIELD_CAPACITY(bitfield32)
const size_t RESERVE_MALLOC_RESERVE_SIZE = RESERVE_MALLOC_SIZE;


static pv_ptr reserved_addr[RESERVE_MALLOC_SIZE];
static bitfield32 reserved_pages;


void reserve_malloc_init()
{
	ASSERT(RESERVE_MALLOC_SIZE <= BITFIELD_CAPACITY(reserved_pages));

	reserved_pages = 0;

	for (size_t i = 0; i < RESERVE_MALLOC_SIZE; i++) {
		ASSERT(!bitfield_get(reserved_pages, i));

		pv_ptr pv = early_kalloc(KPAGE_SIZE, RESERVE_MALLOC_TAG, false, false);

		ASSERT(pv.pa != 0 && ptrs_are_kmapped(pv));
		reserved_addr[i] = pv;

		bitfield_set_high(reserved_pages, i);
	}


	ASSERT(
		reserved_pages ==
		(typeof(reserved_pages))((1ULL << RESERVE_MALLOC_SIZE) - 1));
}


pv_ptr reserve_malloc(const char *new_tag)
{
	for (size_t i = 0; i < RESERVE_MALLOC_SIZE; i++) {
		if (bitfield_get(reserved_pages, i)) {
			pv_ptr pmap = reserved_addr[i];

			DEBUG_ASSERT(ptrs_are_kmapped(pmap));


			if (new_tag) {
#ifdef DEBUG
				const char *v_old_tag = vmalloc_update_tag(pmap.va, new_tag);
				const char *p_old_tag =
					page_allocator_update_tag(pmap.pa, new_tag);
				DEBUG_ASSERT(strcmp(v_old_tag, p_old_tag));
				DEBUG_ASSERT(strcmp(v_old_tag, RESERVE_MALLOC_TAG));
				DEBUG_ASSERT(strcmp(p_old_tag, RESERVE_MALLOC_TAG));
#else
				vmalloc_update_tag(pmap.va, new_tag);
				page_allocator_update_tag(pmap.pa, new_tag);
#endif
			}

			bitfield_clear(reserved_pages, i);

			return pmap;
		}
	}

	PANIC("reserve_malloc: no more reserved pages available");
}


void reserve_malloc_fill()
{
#define RESERVE_IS_FULL()  \
	(reserved_pages == \
	 (typeof(reserved_pages))((1ULL << RESERVE_MALLOC_SIZE) - 1))

	raw_kmalloc_cfg cfg = RAW_KMALLOC_KMAP_CFG;
	cfg.fill_reserve = false;
	cfg.kmap = true;
	cfg.assign_pa = true;

	while (!RESERVE_IS_FULL()) {
		for (size_t i = 0; i < RESERVE_MALLOC_SIZE; i++) {
			if (RESERVE_IS_FULL())
				goto end;

			if (bitfield_get(reserved_pages, i))
				continue;

			v_uintptr va = (v_uintptr)raw_kmalloc(
				1,
				RESERVE_MALLOC_TAG,
				&cfg);  // it can actually get a new idx from the reserve,
			                // thats why the for loop is inside a while
			p_uintptr pa = mm_kva_to_kpa(va);
			pv_ptr pv = pv_ptr_new(pa, va);


			DEBUG_ASSERT(pv.pa != 0 && ptrs_are_kmapped(pv));
			DEBUG_ASSERT(pv.pa % KPAGE_ALIGN == 0);

			reserved_addr[i] = pv;

			memzero64((void*)va, KPAGE_SIZE);

			bitfield_set_high(reserved_pages, i);
		}
	}

end:
	DEBUG_ASSERT(
		reserved_pages ==
		(typeof(reserved_pages))((1ULL << RESERVE_MALLOC_SIZE) - 1));
}
