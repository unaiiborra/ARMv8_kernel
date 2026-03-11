#pragma once

#include <lib/mem.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    struct {
        bool use_kmap;
        p_uintptr_t kmap_pa;
    } kmap;
    bool assing_pa;
    bool device_mem;
    bool permanent;
} vmalloc_cfg;

typedef struct {
    const char* tag;
    bool kmapped;
    bool pa_assigned;
    bool device_mem;
    bool permanent;
} vmalloc_allocated_area_mdt;


/// its just a quick access to the node to avoid the search of the node location.
typedef struct {
    void* rva_;
} vmalloc_token;


vmalloc_token vmalloc_get_token(void* allocation_addr);
v_uintptr_t
vmalloc(size_t pages, const char* tag, vmalloc_cfg cfg, vmalloc_token* t);


/// frees an allocated area, returns the byte count of the area. If provided a
/// not null info, also provides the freed area info
size_t
__vmalloc_token__vfree(vmalloc_token t, vmalloc_allocated_area_mdt* info);
size_t __voidptr__vfree(void* va, vmalloc_allocated_area_mdt* info);
#define vfree(T, info)                                                             \
    _Generic((T), vmalloc_token: __vmalloc_token__vfree, void*: __voidptr__vfree)( \
        T,                                                                         \
        info)


typedef enum {
    VMALLOC_VA_INFO_FREE,
    VMALLOC_VA_INFO_RESERVED,
    VMALLOC_VA_UNREGISTERED,
} vmalloc_va_info_state;


struct vmalloc_pa_mdt; // forward decl

typedef struct {
    vmalloc_allocated_area_mdt info;

    struct {
        struct vmalloc_pa_mdt* list;
        uint32_t count;
    } pa_mdt;
} vmalloc_mdt;

typedef struct {
    vmalloc_va_info_state state;
    union {
        struct {
            v_uintptr_t free_start;
            size_t free_size;
        } free;

        struct {
            v_uintptr_t reserved_start;
            size_t reserved_size;
            vmalloc_mdt mdt;
        } reserved;
    } state_info;
} vmalloc_va_info;


const char* vmalloc_update_tag(v_uintptr_t va, const char* new_tag);

vmalloc_va_info vmalloc_get_addr_info(void* addr);

void vmalloc_debug_free();
void vmalloc_debug_reserved();
void vmalloc_debug_nodes();


size_t vmalloc_get_free_bytes();
size_t vmalloc_get_reserved_bytes();


vmalloc_allocated_area_mdt __vmalloc_token__vmalloc_get_mdt(vmalloc_token t);
vmalloc_allocated_area_mdt __voidptr__vmalloc_get_mdt(void* allocation_addr);
#define vmalloc_get_mdt(T)                               \
    _Generic(                                            \
        (T),                                             \
        vmalloc_token: __vmalloc_token__vmalloc_get_mdt, \
        void*: __voidptr__vmalloc_get_mdt)(T)


typedef struct {
    p_uintptr_t pa;
    v_uintptr_t va;
    size_t order;
} vmalloc_pa_info;

void vmalloc_push_pa(
    vmalloc_token t,
    size_t order,
    p_uintptr_t pa,
    v_uintptr_t va);
size_t vmalloc_get_pa_count(vmalloc_token t);
bool vmalloc_get_pa_info(vmalloc_token t, vmalloc_pa_info* buf, size_t buf_size);
