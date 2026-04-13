#pragma once

#define KERNEL_ADDR_BITS 48
#define KERNEL_BASE      (~((1UL << (KERNEL_ADDR_BITS - 1UL)) - 1UL))

#ifndef __ASSEMBLER__
#    include <arm/mmu.h>
#    include <kernel/mm/page_malloc.h>
#    include <kernel/mm/vmalloc.h>
#    include <kernel/panic.h>
#    include <lib/mem.h>
#    include <lib/unit/mem.h>
#    include <stdbool.h>
#    include <stddef.h>
#    include <stdint.h>

#    define PAGE_SIZE  (MEM_KiB * 4ULL)
#    define PAGE_ALIGN PAGE_SIZE


typedef enum {
    MM_VMEM_LO = 0,
    MM_VMEM_HI = 1,
} mm_valoc;


extern const mmu_allocator      MM_STD_MMU_ALLOCATOR;
extern const mmu_allocator_free MM_STD_MMU_ALLOCATOR_FREE;
extern mmu_mapping* const       MM_MMU_IDENTITY_LO_MAPPING;


extern noreturn void _reset_stack_and_return(void* va_ret_addr);

#    define reset_stack_and_return(ret_addr) _reset_stack_and_return(ret_addr)


void mm_reloc(void* va_ret_addr);


void mm_early_init();

/// it expects to be provided the identity mapping handle. It will free it, and
/// replace it by the kernel mmu handle after relocation
void mm_init();


void mm_dbg_print_mmu();


bool mm_kernel_is_relocated();


static inline puintptr_t kva_to_kpa(vuintptr_t va)
{
    DEBUG_ASSERT((va & ~KERNEL_BASE) == (va - KERNEL_BASE));

    return va & ~KERNEL_BASE;
}

#    define kva_to_kpa_pt(va) (void*)kva_to_kpa((vuintptr_t)(va))

static inline vuintptr_t kpa_to_kva(puintptr_t pa)
{
    return pa | KERNEL_BASE;
}

#    define kpa_to_kva_pt(pa) (void*)kpa_to_kva((puintptr_t)(pa))


static inline bool is_kva_ptr(const void* a)
{
    return (uintptr_t)a >= KERNEL_BASE;
}

static inline bool is_kva_uintptr_t(uintptr_t a)
{
    return a >= KERNEL_BASE;
}


static inline vuintptr_t __as_kva(uintptr_t ptr)
{
    return is_kva_uintptr_t(ptr) ? ptr : kpa_to_kva(ptr);
}

static inline puintptr_t __as_kpa(uintptr_t ptr)
{
    return is_kva_uintptr_t(ptr) ? kva_to_kpa(ptr) : ptr;
}

#    define as_kva(ptr) ((typeof(ptr))__as_kva((uintptr_t)(ptr)))
#    define as_kpa(ptr) ((typeof(ptr))__as_kpa((uintptr_t)(ptr)))


#    define is_kva(a) \
        _Generic((a), void*: is_kva_ptr, uintptr_t: is_kva_uintptr_t)(a)


static inline bool ptrs_are_kmapped(pv_ptr pv)
{
    return (pv.pa | KERNEL_BASE) == pv.va;
}


bool mm_va_is_in_kmap_range(void* ptr);


mmu_mapping      mm_mmu_mapping_new(mmu_tbl_rng rng);
mmu_core_handle* mm_mmu_core_handler_get(uint32_t coreid);
mmu_core_handle* mm_mmu_core_handler_get_self();


typedef struct {
    // if assign_phys == true, the kernel physmap offset is assured (va == pa +
    // KERNEL_BASE), else it is not assured and the phys addr is dynamically
    // assigned
    bool fill_reserve;
    bool assign_pa;
    bool kmap; // if the va must be with an offset of KERNEL_BASE, assing_pa
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


#    define __RAW_KMALLOC_GET_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#    define __RAW_KMALLOC_3(pages, tag, cfg) \
        __raw_kmalloc_null((pages), (tag), (cfg))
#    define __RAW_KMALLOC_4(pages, tag, cfg, info) \
        __raw_kmalloc((pages), (tag), (cfg), (info))

void* __raw_kmalloc(
    size_t                 pages,
    const char*            tag,
    const raw_kmalloc_cfg* cfg,
    raw_kmalloc_info*      info);
static inline void*
__raw_kmalloc_null(size_t pages, const char* tag, const raw_kmalloc_cfg* cfg)
{
    return __raw_kmalloc(pages, tag, cfg, NULL);
}


#    define raw_kmalloc(...)     \
        __RAW_KMALLOC_GET_MACRO( \
            __VA_ARGS__,         \
            __RAW_KMALLOC_4,     \
            __RAW_KMALLOC_3)(__VA_ARGS__)
void raw_kfree(void* ptr);


typedef enum {
    CACHE_8    = 8,
    CACHE_16   = 16,
    CACHE_32   = 32,
    CACHE_64   = 64,
    CACHE_128  = 128,
    CACHE_256  = 256,
    CACHE_512  = 512,
    CACHE_1024 = 1024,
} cache_malloc_size;

void* cache_malloc(cache_malloc_size s);
void  cache_free(cache_malloc_size s, void* ptr);


void* kmalloc(size_t bytes);
void  kfree(void* ptr);

#endif
