#pragma once

#include <arm/mmu.h>
#include <kernel/task.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <stddef.h>
#include <stdint.h>


// WARNING: This module does not allow for partial frees nor considers merging
// same attributed regions, which can cause valid buffers from userspace to be
// considered as not commited or not reserved, for example in a syscall print if
// the string runs through two adjacent regions, it could be detected as a wrong
// operation, when it should be allowed. For that to work a small remodeling of
// vmalloc and page malloc would be required

// NOTE: the following documentation has been made by an LLM

typedef struct uregion uregion_t; // used in task

typedef enum {
    UREGION_OK,
    UREGION_OVERLAPS,
    UREGION_ERROR,
} uregion_result_e;


typedef enum {
    UREGION_F_READ  = 0,
    UREGION_F_WRITE = 1,
    UREGION_F_EXEC  = 2,
} uregion_flags_e;


// ─── reserve ─────────────────────────────────────────────────────────────────

/// Reserves a userspace virtual address range without assigning physical pages.
/// The corresponding kernel VA window is also reserved but not mapped.
/// Intended for anonymous mmap, lazily-loaded ELF segments, etc.
/// Physical pages are assigned on demand via uregion_commit().
///
/// @param t      target task
/// @param usr_va userspace VA start (must be page-aligned)
/// @param pages  number of pages to reserve
/// @param read   userspace read permission
/// @param write  userspace write permission
/// @param exec   userspace execute permission
/// @returns UREGION_OK, UREGION_OVERLAPS if the range collides with an
///          existing region, or UREGION_ERROR on invalid input
uregion_result_e uregion_reserve(
    task_t*   t,
    uintptr_t usr_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      exec);


/// Result type for uregion_reserve_static().
typedef struct {
    uregion_result_e result;
    void* knl_va; ///< kernel VA for the allocated region, NULL on failure
} uregion_reserve_static_result_t;

/// Reserves a userspace VA range and immediately assigns physical pages.
/// Both the userspace and kernel mappings are fully set up on return.
/// Intended for regions that must always be accessible without faulting,
/// such as task-internal structures or kernel-managed stacks.
///
/// @param t      target task
/// @param usr_va userspace VA start (must be page-aligned)
/// @param pages  number of pages to reserve and commit
/// @param read   userspace read permission
/// @param write  userspace write permission
/// @param exec   userspace execute permission
/// @returns result with UREGION_OK and a valid knl_va on success,
///          or UREGION_OVERLAPS / UREGION_ERROR on failure
uregion_reserve_static_result_t uregion_reserve_static(
    task_t*   t,
    uintptr_t usr_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      exec);


// ─── commit ──────────────────────────────────────────────────────────────────

/// Assigns physical pages to an already-reserved range and maps them in both
/// the userspace and kernel page tables.
/// Typically called from the page fault handler when a lazy region is accessed.
///
/// @param t      target task
/// @param usr_va start of the range to commit (must be page-aligned and within
///               an existing reserved region)
/// @param pages  number of pages to commit
/// @returns kernel VA for the committed range, or NULL if the range is not
///          within a reserved region or is already fully mapped
void* uregion_commit(task_t* t, uintptr_t usr_va, uint32_t pages);


// ─── free ────────────────────────────────────────────────────────────────────

/// Frees the userspace region that exactly covers
/// [usr_va, usr_va + pages * PAGE_SIZE). The range must match a reserved
/// region exactly — partial frees are not supported and will return false.
/// Unmaps both userspace and kernel mappings, frees physical pages,
/// and releases the vmalloc reservation.
///
/// @param t      target task
/// @param usr_va start of the region to free
/// @param pages  number of pages to free
/// @returns true on success, false if no matching region was found or the
///          range does not exactly cover a reserved region
bool uregion_free(task_t* t, uintptr_t usr_va, uint32_t pages);


// ─── query ───────────────────────────────────────────────────────────────────

/// Returns true if [start, start + size) is fully contained within a single
/// reserved region.
///
/// @param t          target task
/// @param start      range start (does not need to be page-aligned)
/// @param size       range size in bytes
/// @param out_region if non-NULL and the range is reserved, set to the
///                   containing region
bool uregion_is_reserved(
    task_t*     t,
    uintptr_t   start,
    size_t      size,
    uregion_t** out_region);

/// Returns true if [start, start + size) is fully contained within a reserved
/// region AND every page in the range has a physical address assigned.
///
/// @param t          target task
/// @param start      range start (does not need to be page-aligned)
/// @param size       range size in bytes
/// @param out_region if non-NULL and the range is committed, set to the
///                   containing region
bool uregion_is_committed(
    task_t*     t,
    uintptr_t   start,
    size_t      size,
    uregion_t** out_region);

/// Finds the highest available gap in the userspace address space large enough
/// to hold `pages` pages. The search is top-down to keep low addresses
/// (including the null page) naturally unoccupied.
///
/// @param t     target task
/// @param pages number of contiguous pages needed
/// @returns page-aligned VA of the free gap, or UINTPTR_MAX if none found
uintptr_t uregion_find_free(task_t* t, uint32_t pages);

/// Translates a userspace VA to its corresponding kernel VA within the same
/// region. The kernel window mirrors the userspace region and provides
/// EL1 read/write access to the same physical pages.
///
/// @param region  the region containing usr_va
/// @param usr_va  userspace VA to translate
/// @param knl_va  if non-NULL and the translation succeeds, set to the
///                corresponding kernel VA
/// @returns true if usr_va is within the region and the kernel window exists,
///          false otherwise
bool uregion_get_knl_access(
    uregion_t* region,
    uintptr_t  usr_va,
    uintptr_t* knl_va);

/// Returns the value of a public flag for the given region.
///
/// @param region target region
/// @param flag   flag to query (uregion_flags_e)
bool uregion_get_flag(uregion_t* region, uregion_flags_e flag);
