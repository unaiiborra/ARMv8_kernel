#include "containers.h"

#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/stdbitfield.h>
#include <stddef.h>
#include <stdint.h>

#include "../../init/mem_regions/early_kalloc.h"
#include "../../malloc/internal/reserve_malloc.h"


static vmalloc_container* first_fva_container;
static vmalloc_container* first_rva_container;


typedef enum {
    VMALLOC_FVA,
    VMALLOC_RVA,
} vmalloc_container_enum;


static inline vmalloc_container*
vmalloc_container_new(vmalloc_container* last, vmalloc_container_enum e)
{
    DEBUG_ASSERT(last);

    v_uintptr_t va = reserve_malloc(
                         e == VMALLOC_FVA ? "vmalloc node container fva"
                                          : "vmalloc node container rva")
                         .va;

    DEBUG_ASSERT(last->undef.hdr.next == NULL);
    DEBUG_ASSERT((va & (KPAGE_SIZE - 1)) == 0);

    vmalloc_container* allocated = (vmalloc_container*)va;

    allocated->undef.hdr.next = NULL;
    allocated->undef.hdr.prev = last;
    for (size_t i = 0; i < (DATA_BYTES / sizeof(uint64_t)); i++)
        allocated->undef.data[i] = 0;

    last->undef.hdr.next = allocated;

    return allocated;
}


static inline void*
vmaloc_node_new(vmalloc_container* first, const vmalloc_container_enum e)
{
#ifdef DEBUG
    DEBUG_ASSERT(first);
    size_t x = 0;
#endif
    vmalloc_container* cur = first;
    vmalloc_container* prev = NULL;

    size_t i, j, k;
    rva_node* rva_nodes;
    fva_node* fva_nodes;
    bf* reserved_nodes;
    size_t count = (e == VMALLOC_FVA) ? FVA_NODE_COUNT : RVA_NODE_COUNT;


find:
    while (cur) {
        if (e == VMALLOC_FVA) {
            fva_nodes = &cur->fva.data.nodes[0];
            reserved_nodes = &cur->fva.data.reserved_nodes[0];
        }
        else {
            rva_nodes = &cur->rva.data.nodes[0];
            reserved_nodes = &cur->rva.data.reserved_nodes[0];
        }

        for (i = 0; i < count; i++) {
            j = i / BITFIELD_CAPACITY(bf);
            k = i % BITFIELD_CAPACITY(bf);

            if (!bitfield_get(reserved_nodes[j], k)) {
                // found free node
                bitfield_set_high(reserved_nodes[j], k);

                return (e == VMALLOC_FVA) ? (void*)(fva_nodes + i)
                                          : (void*)(rva_nodes + i);
            }
        }

        prev = cur;
        cur = cur->undef.hdr.next;
    }

#ifdef DEBUG
    DEBUG_ASSERT(x++ == 0);
#endif

    cur = vmalloc_container_new(prev, e); // it updates prev->next inside

    goto find;
}


static inline void
container_free(vmalloc_container* first, vmalloc_container* to_free)
{
    DEBUG_ASSERT(first && to_free);

    vmalloc_container* cur = first;
    while (cur) {
        if (cur->undef.hdr.next == to_free) {
            cur->undef.hdr.next = to_free->undef.hdr.next;

            if (to_free->undef.hdr.next)
                to_free->undef.hdr.next->undef.hdr.prev = cur;

            raw_kfree(to_free);
            return;
        }

        cur = cur->undef.hdr.next;
    }

    PANIC();
}


void vmalloc_init_containers()
{
    first_fva_container = (vmalloc_container*)early_kalloc(
                              sizeof(vmalloc_container),
                              "vmalloc node container (fva first container)",
                              true,
                              false)
                              .va;
    first_rva_container = (vmalloc_container*)early_kalloc(
                              sizeof(vmalloc_container),
                              "vmalloc node container (rva first container)",
                              true,
                              false)
                              .va;


    *first_fva_container = (vmalloc_container) {0};
    *first_rva_container = (vmalloc_container) {0};
}


fva_node* get_new_fva_node()
{
    fva_node* fva = vmaloc_node_new(first_fva_container, VMALLOC_FVA);

    DEBUG_ASSERT((v_uintptr_t)fva % _Alignof(fva_node) == 0);

    return fva;
}


rva_node* get_new_rva_node()
{
    rva_node* rva = vmaloc_node_new(first_rva_container, VMALLOC_RVA);

    DEBUG_ASSERT((v_uintptr_t)rva % _Alignof(rva_node) == 0);

    return rva;
}


void free_fva_node(fva_node* node)
{
    // get container by aligning down to 4096
    vmalloc_container* container =
        (vmalloc_container*)((v_uintptr_t)node & ~(KPAGE_SIZE - 1ULL));

    fva_container_data* d = (fva_container_data*)&container->fva.data;

    size_t i = (size_t)(node - d->nodes);

    bf* reserved_nodes = d->reserved_nodes;

    ASSERT(i < FVA_NODE_COUNT);

    size_t j = i / BITFIELD_CAPACITY(bf);
    size_t k = i % BITFIELD_CAPACITY(bf);


    DEBUG_ASSERT(
        bitfield_get(reserved_nodes[j], k),
        "vmalloc_node_free_fva: double free");

    bitfield_clear(reserved_nodes[j], k);


    // check if the container is empty, and if so, free the full container
    if (container == first_fva_container)
        return;

    for (i = 0; i < BITFIELD_COUNT_FOR(FVA_NODE_COUNT, bf); i++)
        if (reserved_nodes[i] != 0)
            return;

    container_free(first_fva_container, container);
}

void free_rva_node(rva_node* node)
{
    // get container by aligning down to 4096
    vmalloc_container* container =
        (vmalloc_container*)((v_uintptr_t)node & ~(KPAGE_SIZE - 1ULL));

    rva_container_data* c = (rva_container_data*)&container->rva.data;

    size_t i = (size_t)(node - c->nodes);

    DEBUG_ASSERT(node == &c->nodes[i]);

    bf* reserved_nodes = c->reserved_nodes;

    ASSERT(i < RVA_NODE_COUNT);

    size_t j = i / BITFIELD_CAPACITY(bf);
    size_t k = i % BITFIELD_CAPACITY(bf);


    DEBUG_ASSERT(
        bitfield_get(reserved_nodes[j], k),
        "vmalloc_node_free_rva: double free");

    bitfield_clear(reserved_nodes[j], k);


    // check if the container is empty, and if so, free the full container
    if (container == first_rva_container)
        return;

    for (i = 0; i < BITFIELD_COUNT_FOR(RVA_NODE_COUNT, bf); i++)
        if (reserved_nodes[i] != 0)
            return;

    container_free(first_rva_container, container);
}


void vmalloc_containers_debug_fva()
{
    vmalloc_container* c = first_fva_container;
    size_t i = 0;

    while (c) {
        fva_container_data d = c->fva.data;

        kprintf("\n\r[container %d]\n\r", i++);
        size_t j = 0;

        while (j < BITFIELD_COUNT_FOR(FVA_NODE_COUNT, bf)) {
            uint64_t bitfield = d.reserved_nodes[j++];
            kprintf("%b, ", bitfield);
        }

        c = c->fva.hdr.next;
    }
}

void vmalloc_containers_debug_rva()
{
    vmalloc_container* c = first_rva_container;
    size_t i = 0;

    while (c) {
        rva_container_data d = c->rva.data;

        kprintf("\n\r[container %d]\n\r", i++);

        size_t j = 0;

        while (j < BITFIELD_COUNT_FOR(RVA_NODE_COUNT, bf)) {
            uint64_t bitfield = d.reserved_nodes[j++];
            kprintf("%b, ", bitfield);
        }

        c = c->rva.hdr.next;
    }
}
