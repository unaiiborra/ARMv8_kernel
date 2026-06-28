#include <arm/cpu.h>
#include <arm/exceptions/exceptions.h>
#include <arm/smccc/smc.h>
#include <arm/sysregs/sysregs.h>
#include <drivers/gicv3.h>
#include <kernel/embedded_binary.h>
#include <kernel/init.h>
#include <kernel/io/stdio.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/smp.h>
#include <lib/data_structures/kvec.h>
#include <lib/stdattribute.h>
#include <lib/stdmacros.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include "kernel/io/vfs_serial.h"
#include "kernel/mm/cache_malloc.h"
#include "kernel/mm/elf.h"
#include "kernel/mm/page_malloc.h"
#include "kernel/mm/vmalloc.h"
#include "kernel/scheduler.h"
#include "kernel/task.h"
#include "kernel/time.h"
#include "lib/lock.h"
#include "lib/mem.h"
#include "lib/performance_monitor.h"

// #define N_OUTER 200
// #define N_INNER 200
// #define N_TOTAL (N_OUTER * N_INNER)

// typedef enum {
//     METRIC_PAGE_MALLOC = 0,
//     METRIC_PAGE_FREE,
//     METRIC_VMALLOC,
//     METRIC_VFREE,
//     METRIC_CACHE_MALLOC_8,
//     METRIC_CACHE_MALLOC_64,
//     METRIC_CACHE_MALLOC_512,
//     METRIC_CACHE_MALLOC_2048,
//     METRIC_CACHE_FREE_8,
//     METRIC_CACHE_FREE_64,
//     METRIC_CACHE_FREE_512,
//     METRIC_CACHE_FREE_2048,
//     METRIC_KMALLOC_8,
//     METRIC_KMALLOC_1P, // 1 page  (4096 bytes)
//     METRIC_KMALLOC_2P, // 2 pages (8192 bytes)
//     METRIC_KMALLOC_3P, // 3 pages (12288 bytes)
//     METRIC_KMALLOC_5P, // 5 pages (20480 bytes)
//     METRIC_KFREE_8,
//     METRIC_KFREE_1P,
//     METRIC_KFREE_2P,
//     METRIC_KFREE_3P,
//     METRIC_KFREE_5P,
//     METRIC_COUNT,
// } metric_id_e;

// static const char* metric_names[METRIC_COUNT] = {
//     [METRIC_PAGE_MALLOC]       = "page_malloc",
//     [METRIC_PAGE_FREE]         = "page_free",
//     [METRIC_VMALLOC]           = "vmalloc",
//     [METRIC_VFREE]             = "vfree",
//     [METRIC_CACHE_MALLOC_8]    = "cache_malloc_8",
//     [METRIC_CACHE_MALLOC_64]   = "cache_malloc_64",
//     [METRIC_CACHE_MALLOC_512]  = "cache_malloc_512",
//     [METRIC_CACHE_MALLOC_2048] = "cache_malloc_2048",
//     [METRIC_CACHE_FREE_8]      = "cache_free_8",
//     [METRIC_CACHE_FREE_64]     = "cache_free_64",
//     [METRIC_CACHE_FREE_512]    = "cache_free_512",
//     [METRIC_CACHE_FREE_2048]   = "cache_free_2048",
//     [METRIC_KMALLOC_8]         = "kmalloc_8",
//     [METRIC_KMALLOC_1P]        = "kmalloc_1p",
//     [METRIC_KMALLOC_2P]        = "kmalloc_2p",
//     [METRIC_KMALLOC_3P]        = "kmalloc_3p",
//     [METRIC_KMALLOC_5P]        = "kmalloc_5p",
//     [METRIC_KFREE_8]           = "kfree_8",
//     [METRIC_KFREE_1P]          = "kfree_1p",
//     [METRIC_KFREE_2P]          = "kfree_2p",
//     [METRIC_KFREE_3P]          = "kfree_3p",
//     [METRIC_KFREE_5P]          = "kfree_5p",
// };

// static kvec(monitor_data_t) metrics[METRIC_COUNT];

// static inline void record(metric_id_e id)
// {
//     performance_monitor_fence();
//     monitor_data_t in = performance_monitor_read_fence(IMX8MP_CPU_FREQ);
//     kvec_push(&metrics[id], &in);
// }
// static void bench_page_allocator()
// {
//     puintptr_t ptrs[N_INNER];
//     for (size_t i = 0; i < N_OUTER; i++) {
//         irqlocked()
//         {
//             for (size_t j = 0; j < N_INNER; j++) {
//                 performance_monitor_fence();
//                 ptrs[j] = page_malloc(0, (mm_page_data) {});
//                 record(METRIC_PAGE_MALLOC);
//             }
//             for (size_t j = 0; j < N_INNER; j++) {
//                 performance_monitor_fence();
//                 page_free(ptrs[j]);
//                 record(METRIC_PAGE_FREE);
//             }
//         }
//     }
// }

// static void bench_vmalloc()
// {
//     vmalloc_token vtokens[N_INNER];
//     for (size_t i = 0; i < N_OUTER; i++) {
//         irqlocked()
//         {
//             for (size_t j = 0; j < N_INNER; j++) {
//                 performance_monitor_fence();
//                 vmalloc(1, "", (vmalloc_cfg) {}, &vtokens[j]);
//                 record(METRIC_VMALLOC);
//             }
//             for (size_t j = 0; j < N_INNER; j++) {
//                 performance_monitor_fence();
//                 vfree(vtokens[j], NULL);
//                 record(METRIC_VFREE);
//             }
//         }
//     }
// }

// static void bench_cache_malloc()
// {
//     const cache_malloc_size sizes[4] = {
//         CACHE_8,
//         CACHE_64,
//         CACHE_512,
//         CACHE_2048};

//     const metric_id_e metr[4] = {
//         METRIC_CACHE_MALLOC_8,
//         METRIC_CACHE_MALLOC_64,
//         METRIC_CACHE_MALLOC_512,
//         METRIC_CACHE_MALLOC_2048,
//     };

//     const metric_id_e metrfree[4] = {
//         METRIC_CACHE_FREE_8,
//         METRIC_CACHE_FREE_64,
//         METRIC_CACHE_FREE_512,
//         METRIC_CACHE_FREE_2048,
//     };

//     for (size_t c = 0; c < 4; c++) {
//         for (size_t i = 0; i < N_OUTER; i++) {
//             irqlocked()
//             {
//                 void* ptrs[N_INNER];
//                 for (size_t j = 0; j < N_INNER; j++) {
//                     performance_monitor_fence();
//                     ptrs[j] = cache_malloc(sizes[c]);
//                     record(metr[c]);
//                 }
//                 for (size_t j = 0; j < N_INNER; j++) {
//                     performance_monitor_fence();
//                     cache_free(ptrs[j]);
//                     record(metrfree[c]);
//                 }
//             }
//         }
//     }
// }

// static void bench_kmalloc()
// {
//     size_t sizes[5] = {
//         8, // to test the cache malloc path
//         PAGE_SIZE * 1,
//         PAGE_SIZE * 2,
//         PAGE_SIZE * 3,
//         PAGE_SIZE * 5,
//     };

//     static const metric_id_e kmalloc_metrics[5] = {
//         METRIC_KMALLOC_8,
//         METRIC_KMALLOC_1P,
//         METRIC_KMALLOC_2P,
//         METRIC_KMALLOC_3P,
//         METRIC_KMALLOC_5P,
//     };

//     static const metric_id_e kfree_metrics[5] = {
//         METRIC_KFREE_8,
//         METRIC_KFREE_1P,
//         METRIC_KFREE_2P,
//         METRIC_KFREE_3P,
//         METRIC_KFREE_5P,
//     };

//     void* ptrs[N_INNER];
//     for (size_t sz = 0; sz < 5; sz++) {
//         for (size_t i = 0; i < N_OUTER; i++) {
//             irqlocked()
//             {
//                 for (size_t j = 0; j < N_INNER; j++) {
//                     performance_monitor_fence();
//                     ptrs[j] = kmalloc(sizes[sz]);
//                     record(kmalloc_metrics[sz]);
//                 }
//                 for (size_t j = 0; j < N_INNER; j++) {
//                     performance_monitor_fence();
//                     kfree(ptrs[j]);
//                     record(kfree_metrics[sz]);
//                 }
//             }
//         }
//     }
// }

// static void print_metrics()
// {
//     for (size_t i = 0; i < METRIC_COUNT; i++) {
//         performance_monitor_print_metrics(
//             &metrics[i],
//             IMX8MP_CPU_FREQ,
//             metric_names[i]);
//     }
// }

noreturn void kernel_entry()
{
    if (get_cpuid() == 0) {
        if (!mm_kernel_is_relocated())
            kernel_early_init();
        else
            kernel_init();
    }

    if (get_cpuid() != 0)
        loop asm volatile("wfi");


    task_t*   hello = task_new("hello world");
    uintptr_t entry;
    elf_load(
        hello,
        EMBEDDED_BINARY(hello_elf),
        EMBEDDED_BINARY_SIZE(hello_elf),
        &entry);

    schedule_ready_thread(hello, entry);

    scheduler_loop_cpu_enter();

    loop asm volatile("wfi");
}
