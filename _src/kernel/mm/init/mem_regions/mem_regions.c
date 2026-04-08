#include "mem_regions.h"

#include "kernel/mm.h"

// https://github.com/nxp-imx/linux-imx/blob/lf-6.12.y/arch/arm64/boot/dts/freescale/imx8mp.dtsi
// https://github.com/nxp-imx/linux-imx/blob/lf-6.12.y/arch/arm64/boot/dts/freescale/imx8mp-frdm.dts

static const mem_region REGIONS_[] = {
    [0] =
        {
            .tag   = "MMIO",
            .start = 0x0,
            .size  = 0x40000000,
            .type  = MEM_REGION_MMIO,
        },
    [1] =
        {
            .tag   = "DDR",
            .start = 0x40000000,
            .size  = 0xc0000000,
            .type  = MEM_REGION_DDR,
        },
};

static const mem_region RESERVED_[] = {
    [0] =
        {
            .tag   = "nullptr",
            .start = 0x0,
            .size  = PAGE_SIZE,
            .type  = MEM_REGION_RESERVED,
        },
    [1] =
        {
            .tag   = "OP-TEE",
            .start = 0x56000000,
            .size  = 0x2000000,
            .type  = MEM_REGION_RESERVED,
        },
};


const mem_regions MEM_REGIONS = {
    .REG_COUNT = sizeof(REGIONS_) / sizeof(mem_region),
    .REGIONS   = REGIONS_,
};

const mem_regions MEM_REGIONS_RESERVED = {
    .REG_COUNT = sizeof(RESERVED_) / sizeof(mem_region),
    .REGIONS   = RESERVED_,
};
