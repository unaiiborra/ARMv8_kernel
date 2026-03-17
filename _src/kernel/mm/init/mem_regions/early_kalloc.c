/*
 *  The early allocator used by all the other allocators like the page allocator
 * to allocate itself, and also the physical pages struct. When the early phase
 * of initializing the allocators has finished, the allocated pages information
 * is passed to the other allocators to set the initial state of the memory
 * layout of the kernel.
 */

#include "early_kalloc.h"

#include <arm/mmu.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/align.h>
#include <lib/lock.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdbitfield.h>
#include <lib/stdmacros.h>
#include <stddef.h>

#include "../../mm_info.h"
#include "mem_regions.h"

#define N 8
#define NODE_COUNT (N * BITFIELD_COUNT_FOR(N, bitfield64))
_Static_assert(NODE_COUNT % BITFIELD_COUNT_FOR(NODE_COUNT, bitfield64) == 0);


typedef struct ek_node {
    struct ek_node* next;
    early_memreg memreg;
} ek_node;


static ek_node *first_node, *next_allocation_node;
static _Alignas(16) bool early_kalloc_ended;

static early_memreg* memregs_buf;
static size_t memreg_count;


static inline puintptr_t node_start(ek_node* n)
{
    return as_kpa(n->memreg.addr);
}

static inline puintptr_t node_end(ek_node* n)
{
    return as_kpa(n->memreg.addr) + n->memreg.pages * KPAGE_SIZE;
}

static ek_node* alloc_node()
{
    ek_node* n = next_allocation_node--;

    memreg_count++;

    ASSERT((puintptr_t)n >= mm_info_ddr_start());

    return n;
}

static void reserve_memreg(
    early_memreg e,
    bool replace_all
    /* if true replaces any region, else only ddr regions */
)
{
    ASSERT(!e.free);
    e.addr = as_kpa(e.addr);

    puintptr_t end = e.addr + e.pages * KPAGE_SIZE;

    ek_node* c = first_node;

    while (c) {
        puintptr_t c_start = node_start(c);
        puintptr_t c_end = node_end(c);

        if ((replace_all || c->memreg.free) && c_start <= e.addr &&
            end <= c_end) {
            if (c_start == e.addr && c_end == end) {
                c->memreg = e;
                return;
            }

            if (c_start == e.addr) {
                ek_node* tail = alloc_node();

                *tail = (ek_node) {
                    .next = c->next,
                    .memreg = {
                        .addr = end,
                        .pages = (c_end - end) / KPAGE_SIZE,
                        .free = c->memreg.free,
                        .tag = c->memreg.tag,
                        .permanent = c->memreg.permanent,
                        .device_memory = c->memreg.tag,
                    }};


                c->next = tail;
                c->memreg = e;
                return;
            }

            if (c_end == end) {
                ek_node* n = alloc_node();

                *n = (ek_node) {
                    .next = c->next,
                    .memreg = e,
                };

                c->memreg.pages = (e.addr - c_start) / KPAGE_SIZE;
                c->next = n;
                return;
            }

            ek_node* mid = alloc_node();
            ek_node* tail = alloc_node();

            *tail = (ek_node) {
                .next = c->next,
                .memreg = {
                    .addr = end,
                    .pages = (c_end - end) / KPAGE_SIZE,
                    .free = c->memreg.free,
                    .tag = c->memreg.tag,
                    .permanent = c->memreg.permanent,
                    .device_memory = c->memreg.tag,
                }};


            *mid = (ek_node) {
                .next = tail,
                .memreg = e,
            };

            c->memreg.pages = (e.addr - c_start) / KPAGE_SIZE;
            c->next = mid;
            return;
        }

        c = c->next;
    }

    PANIC("reserve_memreg: region not free");
}

void early_kalloc_init()
{
    next_allocation_node =
        (ek_node*)(align_down_pt(mm_info_ddr_end(), _Alignof(ek_node))) - 1;
    first_node = alloc_node();
    first_node->next = NULL;

    ek_node* p = NULL;
    for (size_t i = 0; i < MEM_REGIONS.REG_COUNT; i++) {
        mem_region r = ((mem_region*)pt_as_kpa(MEM_REGIONS.REGIONS))[i];

        ek_node* n = i == 0 ? first_node : alloc_node();

        ASSERT(r.size % KPAGE_SIZE == 0);

        *n = (ek_node) {
            .next = NULL,
            .memreg =
                {
                    .addr = r.start,
                    .pages = div_ceil(r.size, KPAGE_SIZE),
                    .free = r.type == MEM_REGION_DDR ? true : false,
                    .tag = pt_as_kpa(r.tag),
                    .permanent = true,
                    .device_memory = r.type == MEM_REGION_MMIO ? true : false,
                },
        };

        if (p)
            p->next = n;

        p = n;
    }


    for (size_t i = 0; i < MEM_REGIONS_RESERVED.REG_COUNT; i++) {
        mem_region r = pt_as_kpa(MEM_REGIONS_RESERVED.REGIONS)[i];

        reserve_memreg(
            (early_memreg) {
                .addr = r.start,
                .pages = div_ceil(r.size, KPAGE_SIZE),
                .free = false,
                .tag = pt_as_kpa(r.tag),
                .permanent = true,
                .device_memory = false,
            },
            true);
    }


#define RESERVE_SECTION(section)                                           \
    {                                                                      \
        ASSERT(                                                            \
            MM_KSECTIONS.section.start % KPAGE_ALIGN == 0 &&               \
            MM_KSECTIONS.section.size % KPAGE_SIZE == 0 &&                 \
            MM_KSECTIONS.section.end ==                                    \
                (MM_KSECTIONS.section.start + MM_KSECTIONS.section.size)); \
        reserve_memreg(                                                    \
            (early_memreg) {                                               \
                .addr = as_kpa(MM_KSECTIONS.section.start),                \
                .pages = MM_KSECTIONS.section.size / KPAGE_SIZE,           \
                .free = false,                                             \
                .tag = #section,                                           \
                .permanent = true,                                         \
                .device_memory = false,                                    \
            },                                                             \
            false);                                                        \
    }

    RESERVE_SECTION(text);
    RESERVE_SECTION(rodata);
    RESERVE_SECTION(data);
    RESERVE_SECTION(bss);
    RESERVE_SECTION(stacks);
}


pv_ptr
early_kalloc(size_t bytes, const char* tag, bool permanent, bool device_memory)
{
    ASSERT(
        !early_kalloc_ended,
        "early_kalloc: should not be used after calling "
        "early_kalloc_get_memregs()");

    if (!permanent)
        ASSERT(
            is_pow2(bytes / KPAGE_SIZE),
            "early_kalloc: non permanent allocations must be "
            "allocatable as an order to allow later deallocation");

    ek_node* c = first_node;

    while (c) {
        size_t pages = div_ceil(bytes, KPAGE_SIZE);

        if (!c->memreg.free || c->memreg.pages < pages) {
            c = c->next;
            continue;
        }

        puintptr_t addr = c->memreg.addr;

        reserve_memreg(
            (early_memreg) {
                .addr = addr,
                .pages = pages,
                .free = false,
                .tag = pt_as_kva(tag),
                .permanent = permanent,
                .device_memory = device_memory,
            },
            false);

        return pv_ptr_new(addr, as_kva(addr));
    }

    PANIC("early_kalloc: no available free memory");
}


void early_kalloc_get_memregs(early_memreg** mregs, size_t* mreg_struct_count)
{
    bool first_call = !early_kalloc_ended;

    if (first_call) {
        memregs_buf = (early_memreg*)early_kalloc(
                          memreg_count * sizeof(early_memreg),
                          "early_kalloc memregs",
                          false,
                          false)
                          .va;

        size_t i = 0;
        ek_node* c = first_node;

        while (c) {
            memregs_buf[i++] = c->memreg;
            c = c->next;
        }

        DEBUG_ASSERT(i == memreg_count);

        early_kalloc_ended = true;
    }


    *mregs = memregs_buf;
    *mreg_struct_count = memreg_count;


    // make all tags valid kernel virtual addresses
    for (size_t i = 0; i < memreg_count; i++)
        memregs_buf[i].tag = pt_as_kva(memregs_buf[i].tag);
}


void early_kalloc_debug()
{
    bool relocated = mm_kernel_is_relocated();

    ek_node* c = first_node;

    kprint("\n\r[early_kalloc] dump begin");

    while (c) {
        kprintf(
            "\n\r\tnode=%p addr=%p size=%p, tag=%s, free=%s",
            (void*)c,
            (void*)c->memreg.addr,
            c->memreg.pages * KPAGE_SIZE,
            relocated ? pt_as_kva(c->memreg.tag) : pt_as_kpa(c->memreg.tag),
            c->memreg.free ? "true" : "false");
        c = c->next;
    }

    kprint("\n\r[early_kalloc] dump end");
}
