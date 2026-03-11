#include "mm_info.h"

#include <frdm_imx8mp.h>
#include <kernel/mm.h>
#include <kernel/panic.h>
#include <lib/math.h>
#include <lib/mem.h>
#include <stdbool.h>
#include <stddef.h>

#include "init/mem_regions/mem_regions.h"


extern v_uintptr_t __text_start[];
extern v_uintptr_t __text_end[];
extern size_t __text_size[];

extern v_uintptr_t __rodata_start[];
extern v_uintptr_t __rodata_end[];
extern size_t __rodata_size[];

extern v_uintptr_t __data_start[];
extern v_uintptr_t __data_end[];
extern size_t __data_size[];

extern v_uintptr_t __bss_start[];
extern v_uintptr_t __bss_end[];
extern size_t __bss_size[];

extern v_uintptr_t __stacks_start[];
extern v_uintptr_t __stacks_end[];
extern size_t __stacks_size[];

extern v_uintptr_t __heap_start[];
extern v_uintptr_t __heap_end[];
extern size_t __heap_size[];


#define BUILD_KSECTION(section)                    \
    (mm_ksection)                                  \
    {                                              \
        .start = (v_uintptr_t)__##section##_start, \
        .end = (v_uintptr_t)__##section##_end,     \
        .size = (size_t)__##section##_size,        \
    }

const mm_ksections MM_KSECTIONS = (mm_ksections) {
    .text = BUILD_KSECTION(text),
    .rodata = BUILD_KSECTION(rodata),
    .data = BUILD_KSECTION(data),
    .bss = BUILD_KSECTION(bss),
    .stacks = BUILD_KSECTION(stacks),
};


static size_t mm_ddr_size_;
static p_uintptr_t mm_ddr_start_;
static p_uintptr_t mm_ddr_end_;
static p_uintptr_t mm_kernel_start_;
static size_t mm_page_count_;
static size_t mm_addr_space_;


static bool is_valid_ksection(mm_ksection k)
{
    if (k.start + k.size != k.end)
        return false;

    if (as_kpa(k.end) > mm_addr_space_)
        return false;

    return true;
}


/// all this definitions are mostly hardcoded for the imx8mp, the idea is that
/// they should come from a dtb file or any other valid source that allows the
/// kernel to dynamically initialize itself
void mm_info_init()
{
    extern void _start();

    bool ddr_declared = false;
    mm_addr_space_ = 0x0;


    for (size_t i = 0; i < MEM_REGIONS.REG_COUNT; i++) {
        mem_region r =
            ((const mem_region* const)pt_as_kpa(MEM_REGIONS.REGIONS))[i];

        switch (r.type) {
            case MEM_REGION_RESERVED:
                PANIC("Reserved regions must be declared at MEM_REGIONS_RESERVED");
                break;
            case MEM_REGION_DDR:
                ASSERT(!ddr_declared, "only one DDR region must be declared");

                mm_ddr_start_ = r.start;
                mm_ddr_end_ = r.start + r.size;
                mm_ddr_size_ = r.size;

                ddr_declared = true;
                break;
            case MEM_REGION_MMIO:
                break;
        }

        mm_addr_space_ = max(r.start + r.size, mm_addr_space_);
    }

    mm_kernel_start_ = (p_uintptr_t)_start;
    ASSERT(mm_kernel_start_ >= mm_ddr_start_ && mm_kernel_start_ < mm_ddr_end_);

    mm_page_count_ = div_ceil(mm_addr_space_, KPAGE_SIZE);


#define VALIDATE_KSECTION(ksection) \
    ASSERT(is_valid_ksection(MM_KSECTIONS.ksection))
    VALIDATE_KSECTION(text);
    VALIDATE_KSECTION(rodata);
    VALIDATE_KSECTION(data);
    VALIDATE_KSECTION(bss);
    VALIDATE_KSECTION(stacks);
#undef VALIDATE_KSECTION
}


size_t mm_info_ddr_size(void)
{
    return mm_ddr_size_;
}

p_uintptr_t mm_info_ddr_start(void)
{
    return mm_ddr_start_;
}

p_uintptr_t mm_info_ddr_end(void)
{
    return mm_ddr_end_;
}

p_uintptr_t mm_info_kernel_start(void)
{
    return mm_kernel_start_;
}

size_t mm_info_page_count(void)
{
    return mm_page_count_;
}

size_t mm_info_mm_addr_space(void)
{
    return mm_addr_space_;
}
