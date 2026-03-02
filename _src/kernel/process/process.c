#include <kernel/process/embedded_examples.h>
#include <kernel/process/process.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>

#include "arm/mmu.h"
#include "elf.h"
#include "kernel/lib/kvec.h"
#include "kernel/mm.h"
#include "kernel/mm/vmalloc.h"
#include "kernel/panic.h"
#include "lib/math.h"
#include "lib/mem.h"

static uint64 pid_counter;

static void process_add_thread(
    proc* usr_proc,
    v_uintptr entry,
    size_t stack_pages,
    uint64 thread_arg)
{
    pthread pth;
    pthread* p_pth = &pth;

    bool found_null = false;
    size_t n = kvec_len(usr_proc->threads.pthreads);
    for (size_t i = 0; i < n; i++) {
        pthread* ptr;
        bool res = kvec_get_mut(&usr_proc->threads.pthreads, i, (void**)&ptr);
        ASSERT(res);

        if (ptr->pc == 0) {
            // null thread (deleted previously)
            found_null = true;
            p_pth = ptr;
            break;
        }
    }


    size_t stack_size = stack_pages * KPAGE_SIZE;
    v_uintptr stack_bottom;


    *p_pth = (pthread) {
        .th_id = __atomic_add_fetch(
            &usr_proc->threads.pthread_counter,
            1,
            __ATOMIC_SEQ_CST),
        .th_aff = {.core_id = ~0},
        .pc = entry,
        .stack =
            {
                .stack_bottom = 0,
                .stack_size = 0,
            },
        .tpidr_el0 = 0,
        .regs = {0},
    };


    if (!found_null) {
        kvec_push(&usr_proc->threads.pthreads, &pth);
    }
}


void* process_malloc_usr_region(
    proc* usr_proc,
    v_uintptr usr_va,
    size_t pages,
    mmu_pg_cfg* usr_mmu_cfg)
{
    ASSERT(pages > 0);

    raw_kmalloc_info rkinfo;
    void* k_va = raw_kmalloc(
        KPAGE_SIZE,
        usr_proc->PNAME,
        &RAW_KMALLOC_DYNAMIC_CFG,
        &rkinfo);

    DEBUG_ASSERT(rkinfo.raw_kmalloc_type == RAW_KMALLOC_DYNAMIC);

    size_t pa_n = vmalloc_get_pa_count(rkinfo.info.dynamic.vtoken);
    vmalloc_pa_info pa_info[pa_n]; // ordered by va

    bool res = vmalloc_get_pa_info(rkinfo.info.dynamic.vtoken, pa_info, pa_n);
    ASSERT(res);

    uint64 offset = 0;
    for (size_t i = 0; i < pa_n; i++) {
        size_t size = power_of2(pa_info[i].order) * KPAGE_SIZE;

        mmu_map_result map_res = mmu_map(
            &usr_proc->map.usr_mapping,
            usr_va + offset,
            pa_info[i].pa,
            size,
            *usr_mmu_cfg,
            NULL);

        ASSERT(map_res == MMU_MAP_OK);
        DEBUG_ASSERT((v_uintptr)k_va + offset == pa_info[i].va);

        offset += size;
    }

    // save the mapping data into the vector
    size_t n = kvec_len(usr_proc->map.pmap_info);
    for (size_t i = 0; i < n; i++) {
        // try to find a null proc_map_info (previously freed)
        proc_map_info* info;
        bool res = kvec_get_mut(&usr_proc->map.pmap_info, i, (void**)&info);
        ASSERT(res);

        // if pages == 0 is a null proc_map_info
        if (info->pages == 0) {
            *info = (proc_map_info) {
                .k_va = (v_uintptr)k_va,
                .u_va = usr_va,
                .pages = pages,
            };

            return k_va;
        }
    }

    // there is no null proc_map_info for overwriting so just push a new
    // proc_map_info
    kvec_push(
        &usr_proc->map.pmap_info,
        &(proc_map_info) {
            .k_va = (v_uintptr)k_va,
            .u_va = usr_va,
            .pages = pages,
        });

    return k_va;
}

bool usr_proc_new(
    proc* out,
    void* elf,
    size_t elf_size,
    const char* pname,
    size_t stack_pages)
{
    *out = (proc) {
        .pid = __atomic_add_fetch(&pid_counter, 1, __ATOMIC_SEQ_CST),
        .PNAME = pname,
        .threads =
            {
                .pthread_id_counter = 0,
                .pthread_count = 0,
                .pthreads = kvec_new(pthread*),
            },
        .entry = 0x0,
        .map =
            {
                .pmap_info = kvec_new(proc_map_info),
                .usr_mapping = mm_mmu_mapping_new(MMU_LO),
            },
    };

    elf_load_result elfres = elf_load(elf, elf_size, out);
    ASSERT(elfres == ELF_LOAD_OK);

    return true;
}

void usr_proc_resume(proc* uproc)
{
    mmu_core_handle* ch0 = mm_mmu_core_handler_get_self();
    bool res = mmu_core_set_mapping(ch0, &uproc->mmu_map);
}
