#include "mem_regions.h"

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
            .size  = 0x100000000, // 4 GiB
            .type  = MEM_REGION_DDR,
        },
    [2] =
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
