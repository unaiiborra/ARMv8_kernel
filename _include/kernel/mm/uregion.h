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

// WARNING: ALWAYS LOCK THE OWNER TASK BEFORE CALLING ANY OF THESE FUNCTIONS

typedef struct uregion uregion_t; // used in task

typedef enum {
    UREGION_OK,
    UREGION_OVERLAPS,
    UREGION_ERROR,
} uregion_reserve_e;


typedef enum {
    UREGION_F_READ  = 1 << 0,
    UREGION_F_WRITE = 1 << 1,
    UREGION_F_EXEC  = 1 << 2,
} uregion_flags_e;

#define UREGION_REQUIRED_FLAGS_IGNORE  0
#define UREGION_FORBIDDEN_FLAGS_IGNORE 0

typedef enum {
    UREGION_ACCESS_OK,
    UREGION_ACCESS_NOT_RESERVED,  // some or none pages are reserved
    UREGION_ACCESS_NOT_COMMITTED, // reserved but not commited fully
    UREGION_ACCESS_NO_PERMISSION, // required flags not set
} uregion_access_e;


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
uregion_reserve_e uregion_reserve(
    task_t*   t,
    uintptr_t usr_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      exec);


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
uregion_reserve_e uregion_reserve_static(
    task_t*   t,
    uintptr_t usr_va,
    uint32_t  pages,
    bool      read,
    bool      write,
    bool      exec,
    bool      zeroed);


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
void uregion_commit(task_t* t, uintptr_t usr_va, uint32_t pages, bool zeroed);


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



/// Checks if [start, start+size) is fully reserved, commited and has the
/// required flags. Checks multiple regions, not only one region. If any page is
/// reserved but not commited, it will be commited and zeroed when
/// commit_on_demand == true.
uregion_access_e uregions_check_access(
    task_t*         t,
    uintptr_t       start,
    size_t          size,
    uregion_flags_e required_flags,
    uregion_flags_e forbidden_flags,
    bool            commit_on_demand);


/// Finds the highest available gap in the userspace address space large enough
/// to hold `pages` pages. The search is top-down to keep low addresses
/// (including the null page) naturally unoccupied.
///
/// @param t     target task
/// @param pages number of contiguous pages needed
/// @param out   set to the page-aligned VA of the free gap on success
/// @returns     true if a suitable gap was found, false otherwise
bool uregion_find_free(task_t* t, uint32_t pages, uintptr_t* out);

/// Returns the flags of the region, the user must check against uregion_flags_e
///
/// @param region target region
/// @returns flags
uint32_t uregion_get_flags(uregion_t* region);


typedef enum {
    UMEMCPY_KNL_TO_USR,
    UMEMCPY_USR_TO_KNL,
} umemcpy_type_e;

uregion_access_e umemcpy(
    task_t*         t,
    void*           dst,
    const void*     src,
    size_t          size,
    uregion_flags_e required_flags,
    uregion_flags_e forbidden_flags,
    bool            commit_on_demand,
    umemcpy_type_e  type);

uregion_access_e umemzero(
    task_t*         t,
    void*           usr_dst,
    size_t          size,
    uregion_flags_e required_flags,
    uregion_flags_e forbidden_flags,
    bool            commit_on_demand);

void uregion_flush_icache(task_t* t, uintptr_t usr_start, size_t size);
