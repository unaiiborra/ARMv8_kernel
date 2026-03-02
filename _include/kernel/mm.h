#pragma once

#define KERNEL_ADDR_BITS    48
#define KERNEL_BASE         (~((1ULL << (KERNEL_ADDR_BITS - 1)) - 1))

#ifndef __ASSEMBLER__
#    include <kernel/panic.h>
#    include <lib/mem.h>
#    include <lib/stdbool.h>
#    include <lib/stdint.h>
#    include <lib/unit/mem.h>
#    include <arm/mmu.h>

#    include <kernel/mm/vmalloc.h>
#    include <kernel/mm/page_malloc.h>

#    define KPAGE_SIZE     (MEM_KiB * 4ULL)
#    define KPAGE_ALIGN    KPAGE_SIZE


typedef enum {
	MM_VMEM_LO      = 0,
	MM_VMEM_HI      = 1,
} mm_valoc;


void mm_early_init();

/// it expects to be provided the identity mapping handle. It will free it, and
/// replace it by the kernel mmu handle after relocation
void mm_init();


void mm_dbg_print_mmu();


bool mm_kernel_is_relocated();


static inline p_uintptr mm_kva_to_kpa(v_uintptr va)
{
	DEBUG_ASSERT((va & ~KERNEL_BASE) == (va - KERNEL_BASE));

	return va & ~KERNEL_BASE;
}

#    define mm_kva_to_kpa_ptr(va)    (void *)mm_kva_to_kpa((v_uintptr)(va))

static inline v_uintptr mm_kpa_to_kva(p_uintptr pa)
{
	return pa | KERNEL_BASE;
}

#    define mm_kpa_to_kva_ptr(pa)    (void *)mm_kpa_to_kva((p_uintptr)(pa))


static inline bool mm_is_kva_ptr(const void *a)
{
	return (uintptr)a >= KERNEL_BASE;
}

static inline bool mm_is_kva_uintptr(uintptr a)
{
	return a >= KERNEL_BASE;
}


static inline v_uintptr mm_as_kva(uintptr ptr)
{
	return mm_is_kva_uintptr(ptr) ? ptr : mm_kpa_to_kva(ptr);
}

static inline p_uintptr mm_as_kpa(uintptr ptr)
{
	return mm_is_kva_uintptr(ptr) ? mm_kva_to_kpa(ptr) : ptr;
}

#    define mm_as_kva_ptr(ptr)    ((typeof(ptr))mm_as_kva((uintptr)ptr))
#    define mm_as_kpa_ptr(ptr)    ((typeof(ptr))mm_as_kpa((uintptr)ptr))


#    define mm_is_kva(a)                              \
	_Generic((a), void *: mm_is_kva_ptr, uintptr: \
		 mm_is_kva_uintptr)(a)


static inline bool ptrs_are_kmapped(pv_ptr pv)
{
	return (pv.pa | KERNEL_BASE) == pv.va;
}


bool mm_va_is_in_kmap_range(void *ptr);


mmu_mapping mm_mmu_mapping_new(mmu_tbl_rng rng);
mmu_core_handle * mm_mmu_core_handler_get(uint32 coreid);
mmu_core_handle * mm_mmu_core_handler_get_self();


typedef struct {
	// if assign_phys == true, the kernel physmap offset is assured (va == pa +
	// KERNEL_BASE), else it is not assured and the phys addr is dynamically
	// assigned
	bool fill_reserve;
	bool assign_pa;
	bool kmap;      // if the va must be with an offset of KERNEL_BASE, assing_pa
	                // must be true or it will panic
	// if the reserve allocator should be filled after the allocation occurs.
	bool device_mem;
	bool permanent;
	bool init_zeroed;
	// TODO: add more cfgs if needed (for example mmu_cfg)
} raw_kmalloc_cfg;

typedef struct {
	enum {
		RAW_KMALLOC_KMAP,
		RAW_KMALLOC_DYNAMIC,
	} raw_kmalloc_type;
	
	const mmu_pg_cfg* MMU_CFG;

	union {

		struct {
			pv_ptr pv;
			size_t order;
		} kmap;

		struct {
			vmalloc_token vtoken;
		} dynamic;

	} info;
} raw_kmalloc_info;

extern const raw_kmalloc_cfg RAW_KMALLOC_KMAP_CFG;
extern const raw_kmalloc_cfg RAW_KMALLOC_DYNAMIC_CFG;



#define __RAW_KMALLOC_GET_MACRO(_1,_2,_3,_4,NAME,...) NAME
#define __RAW_KMALLOC_3(pages, tag, cfg) \
	__raw_kmalloc_null((pages), (tag), (cfg))
#define __RAW_KMALLOC_4(pages, tag, cfg, info) \
	__raw_kmalloc((pages), (tag), (cfg), (info))

void * __raw_kmalloc(size_t pages, const char *tag, const raw_kmalloc_cfg *cfg, raw_kmalloc_info* info);
static inline void * __raw_kmalloc_null(size_t pages, const char *tag, const raw_kmalloc_cfg *cfg) {
	return __raw_kmalloc(pages, tag, cfg, NULL);
}


#define raw_kmalloc(...) \
	__RAW_KMALLOC_GET_MACRO(__VA_ARGS__, \
				__RAW_KMALLOC_4,                 \
				__RAW_KMALLOC_3                  \
				)(__VA_ARGS__)
void raw_kfree(void *ptr);


typedef enum {
	CACHE_8         = 8,
	CACHE_16        = 16,
	CACHE_32        = 32,
	CACHE_64        = 64,
	CACHE_128       = 128,
	CACHE_256       = 256,
	CACHE_512       = 512,
	CACHE_1024      = 1024,
} cache_malloc_size;

void * cache_malloc(cache_malloc_size s);
void cache_free(cache_malloc_size s, void *ptr);


void * kmalloc(size_t bytes);
void kfree(void *ptr);

#endif
