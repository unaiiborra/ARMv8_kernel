#include <arm/cpu.h>
#include <arm/mmu.h>
#include <kernel/lib/kvec.h>
#include <kernel/lib/smp.h>
#include <kernel/mm.h>
#include <kernel/mm/vmalloc.h>
#include <kernel/panic.h>
#include <kernel/process/embedded_examples.h>
#include <kernel/process/process.h>
#include <kernel/process/thread.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>
#include <lib/stdmacros.h>
#include <lib/unit/mem.h>

#include "../elf/elf.h"
#include "../thread/thread.h"


static const mmu_pg_cfg USR_PROC_STACK_MMU_CFG = (mmu_pg_cfg) {
    .attr_index = 0,
    .ap = MMU_AP_EL0_RW_EL1_RW,
    .shareability = MMU_SH_INNER_SHAREABLE,
    .non_secure = false,
    .access_flag = 1,
    .pxn = 1, // el1 execute never
    .uxn = 1, // el0 execute never
    .sw = 0,
};


void* process_malloc_usr_region(
    proc* usr_proc,
    v_uintptr usr_va,
    size_t pages,
    const mmu_pg_cfg* usr_mmu_cfg)
{
    ASSERT(pages > 0);

    raw_kmalloc_info rkinfo;
    void* knl_va = raw_kmalloc(
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
        DEBUG_ASSERT((v_uintptr)knl_va + offset == pa_info[i].va);

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
                .knl_va = (v_uintptr)knl_va,
                .usr_va = usr_va,
                .pages = pages,
            };

            return knl_va;
        }
    }

    // there is no null proc_map_info for overwriting so just push a new
    // proc_map_info
    kvec_push(
        &usr_proc->map.pmap_info,
        &(proc_map_info) {
            .knl_va = (v_uintptr)knl_va,
            .usr_va = usr_va,
            .pages = pages,
            .read = usr_mmu_cfg->ap == MMU_AP_EL0_RO_EL1_RO ||
                    usr_mmu_cfg->ap == MMU_AP_EL0_RW_EL1_RW,
            .write = usr_mmu_cfg->ap == MMU_AP_EL0_RW_EL1_RW,
            .execute = !usr_mmu_cfg->uxn,
        });

    return knl_va;
}

void process_add_thread_ref(proc* usr_proc, thread* pth)
{
    bool res = false;
    size_t n = kvec_len(usr_proc->threads.pthreads);

    for (size_t i = 0; i < n; i++) {
        thread* p_pth;

        res = kvec_get_copy(&usr_proc->threads.pthreads, i, &p_pth);
        ASSERT(res);

        if (p_pth == NULL) {
            res = kvec_set(&usr_proc->threads.pthreads, i, &pth, NULL);
            ASSERT(res);
            return;
        }
    }

    isize_t psh_res = kvec_push(&usr_proc->threads.pthreads, &pth);
    ASSERT(psh_res != -1);
}

void process_remove_thread_ref(proc* usr_proc, thread* pth)
{
    size_t n = kvec_len(usr_proc->threads.pthreads);
    for (size_t i = 0; i < n; i++) {
        thread* th;
        kvec_get_copy(&usr_proc->threads.pthreads, i, &th);

        if (th == pth) {
            if (i == n - 1) {
                __attribute((unused)) isize_t res =
                    kvec_pop(&usr_proc->threads.pthreads, NULL);
                DEBUG_ASSERT(res == (isize_t)i);

                return;
            }

            thread* in = NULL;
            kvec_set(&usr_proc->threads.pthreads, i, &in, NULL);

            return;
        }
    }

    PANIC("process did not have the thread ptr");
}


static v_uintptr find_stack_region(proc* p, size_t stack_pages)
{
#define USR_ADDR_END (1ULL << (KERNEL_ADDR_BITS - 1))

    size_t stack_size = stack_pages * KPAGE_SIZE;
    v_uintptr start = USR_ADDR_END - stack_size;

retry:

    size_t n = kvec_len(p->map.pmap_info);
    for (size_t i = 0; i < n; i++) {
        proc_map_info* info;
        bool res = kvec_get_mut(&p->map.pmap_info, i, (void**)&info);
        ASSERT(res);

        v_uintptr map_start = info->usr_va;
        v_uintptr map_end = info->usr_va + info->pages * KPAGE_SIZE;

        v_uintptr stack_start = start;
        v_uintptr stack_end = start + stack_size;

        if (stack_end > map_start && stack_start < map_end) {
            start -= stack_size;
            goto retry;
        }
    }

    return start;
}


/*
    public:
*/
uint64 pid_counter;
kvec_T(proc*) procs;


bool proc_get_usrmap_info(proc* pr, v_uintptr usrva, proc_map_info* out)
{
    const size_t N = kvec_len(pr->map.pmap_info);

    for (size_t i = 0; i < N; i++) {
        const proc_map_info* info;
        bool res = kvec_get_mut(&pr->map.pmap_info, i, (void**)&info);
        DEBUG_ASSERT(res);

        if (usrva >= info->usr_va &&
            usrva <= info->usr_va + info->pages * KPAGE_SIZE) {
            *out = *info;
            return true;
        }
    }

    return false;
}


void usr_proc_ctrl_init()
{
    pid_counter = 0;
    procs = kvec_new(proc*);
}

bool usr_proc_new(
    proc** out,
    void* elf,
    size_t elf_size,
    const char* pname,
    bool acquire_thread)
{
    proc* p = kmalloc(sizeof(proc));

    *p = (proc) {
        .pid = __atomic_add_fetch(&pid_counter, 1, __ATOMIC_SEQ_CST),
        .PNAME = pname,
        .entry = 0x0,
        .threads =
            {
                .pthread_id_counter = 0,
                .pthread_count = 0,
                .pthreads = kvec_new(thread*),
            },
        .map =
            {
                .pmap_info = kvec_new(proc_map_info),
                .usr_mapping = mm_mmu_mapping_new(MMU_LO),
            },
    };


    elf_load_result elfres = elf_load(elf, elf_size, p);
    ASSERT(elfres == ELF_LOAD_OK);


    thread* th =
        acquire_thread ? thread_new(p, true) : thread_new_locked_unaquired(p);

    thread_ctx* th_ctx = th->th_ctx;
    DEBUG_ASSERT(th_ctx->owner == p);


    // TODO: allow configurable stack sizes
    size_t stack_size = MEM_MiB * 2;
    size_t stack_guard = MEM_MiB * 2;
    v_uintptr stack_bottom = find_stack_region(p, stack_size + stack_guard * 2);
    stack_bottom += stack_guard; // stack guard of 1 MiB


    process_malloc_usr_region(
        p,
        stack_bottom + MEM_MiB,
        (MEM_MiB * 2) / KPAGE_SIZE,
        &USR_PROC_STACK_MMU_CFG);

    *th_ctx = (thread_ctx) {
        .th_id = th_ctx->th_id,
        .th_aff = get_core_id(),
        .owner = th_ctx->owner,
        .pc = p->entry,
        .stack =
            {
                .stack_bottom = stack_bottom,
                .stack_size = MEM_MiB * 2,
            },
        .tpidr_el0 = 0x0,
        .regs =
            {
                .sp = stack_bottom + stack_size,
            },
    };

    *out = p;

    if (!acquire_thread)
        thread_unlock(th);

    return true;
}
