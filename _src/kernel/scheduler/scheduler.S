#include <arm/exceptions/ctx.h>

#ifndef ECTX_IMPL
.macro ectx_load reg_pt
.endm
.error "<arm/exceptions/exceptions.h> not imported"
#endif

.section .text
// typedef struct {
//     sysreg_t fpcr;
//     sysreg_t fpsr;
//     sysreg_t elr;
//     sysreg_t spsr;

//     sp_t sp_elx;

//     gpr_t x[XREG_COUNT];     // x0-x30
//     vreg128_t v[VREG_COUNT]; // v0-v31
// } arm_ectx;

// extern void _scheduler_loop_cpu_enter(arm_ctx* el0_ctx, arm_ctx* el1_ctx); 
.global _scheduler_loop_cpu_enter
_scheduler_loop_cpu_enter: 
    // use x0 as the *el1_ctx, as this is not an interrupt ctx store,
    // does not really matter if x0-x17 are not stored as they are caller saved.
    // the only important part is to mantain the c abi
    
    stp xzr, x18,   [x1, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 17)]
    stp x19, x20,   [x1, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 19)]
    stp x21, x22,   [x1, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 21)]
    stp x23, x24,   [x1, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 23)]
    stp x25, x26,   [x1, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 25)]
    stp x27, x28,   [x1, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 27)]
    stp x29, x30,   [x1, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 29)]
    
    // store sysregs
    mrs x9, fpcr
    mrs x10, fpsr
    stp x9, x10,    [x1, #ECTX_OFFS_FPCR]

    mrs x12, spsr_el1
    stp x30, x12,   [x1, #ECTX_OFFS_ELR] // set the lr as the elr_el1 for returning as if returning from this fn

    mov x13, sp
    str x13,        [x1, #ECTX_OFFS_SP]

    // store vregs
    add x14, x1, #ECTX_OFFS_V

    st1 {v0.16b,  v1.16b},  [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v2.16b,  v3.16b},  [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v4.16b,  v5.16b},  [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v6.16b,  v7.16b},  [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v8.16b,  v9.16b},  [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v10.16b, v11.16b}, [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v12.16b, v13.16b}, [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v14.16b, v15.16b}, [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v16.16b, v17.16b}, [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v18.16b, v19.16b}, [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v20.16b, v21.16b}, [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v22.16b, v23.16b}, [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v24.16b, v25.16b}, [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v26.16b, v27.16b}, [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v28.16b, v29.16b}, [x14], #(2 * ECTX_SIZEOF_V)
    st1 {v30.16b, v31.16b}, [x14], #(2 * ECTX_SIZEOF_V)

    ectx_load x0
    eret

    

// extern void _scheduler_loop_cpu_exit(arm_ctx* el1_ctx);
.global _scheduler_loop_cpu_exit
_scheduler_loop_cpu_exit:
    ectx_load x0
    eret
    
