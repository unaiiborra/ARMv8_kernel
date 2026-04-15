#ifndef ECTX_IMPL
.error "this file should not be imported directly, import <arm/exceptions/ctx.h> instead"
#endif

.equ ECTX_SIZE,            ARM_STRUCT_ECTX_SIZE

.equ ECTX_OFFS_SP,         ARM_STRUCT_ECTX_OFFS_SP_ELX
.equ ECTX_OFFS_FPCR,       ARM_STRUCT_ECTX_OFFS_FPCR
.equ ECTX_OFFS_FPSR,       ARM_STRUCT_ECTX_OFFS_FPSR
.equ ECTX_OFFS_ELR,        ARM_STRUCT_ECTX_OFFS_ELR
.equ ECTX_OFFS_SPSR,       ARM_STRUCT_ECTX_OFFS_SPSR

.equ ECTX_OFFS_X,          ARM_STRUCT_ECTX_OFFS_X
.equ ECTX_OFFS_V,          ARM_STRUCT_ECTX_OFFS_V

.equ ECTX_SIZEOF_X,        ARM_STRUCT_ECTX_SIZEOF_X
.equ ECTX_SIZEOF_V,        ARM_STRUCT_ECTX_SIZEOF_V

// typedef struct {
//     sysreg_t fpcr;
//     sysreg_t fpsr;
//     sysreg_t elr;
//     sysreg_t spsr;

//     sp_t sp_elx;

//     gpr_t x[XREG_COUNT];     // x0-x30
//     vreg128_t v[VREG_COUNT]; // v0-v31
// } arm_ectx;

.macro ectx_store
    // store xregs
    sub sp, sp, #ECTX_SIZE

    str x0,         [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 0)]
    stp x1, x2,     [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 1)]
    stp x3, x4,     [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 3)]
    stp x5, x6,     [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 5)]
    stp x7, x8,     [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 7)]
    stp x9, x10,    [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 9)]
    stp x11, x12,   [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 11)]
    stp x13, x14,   [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 13)]
    stp x15, x16,   [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 15)]
    stp x17, x18,   [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 17)]
    stp x19, x20,   [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 19)]
    stp x21, x22,   [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 21)]
    stp x23, x24,   [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 23)]
    stp x25, x26,   [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 25)]
    stp x27, x28,   [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 27)]
    stp x29, x30,   [sp, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 29)]


    // store sysregs
    mrs x0, fpcr
    mrs x1, fpsr
    stp x0, x1,     [sp, #ECTX_OFFS_FPCR]

    mrs x2, elr_el1
    mrs x3, spsr_el1
    stp x2, x3,     [sp, #ECTX_OFFS_ELR]

    
    // store sp
#ifdef DEBUG
    // check that we are using sp_el1
    mrs  x0, spsel
    cbnz x0, 1f
0:  wfi
    b 0b
1:
#endif

    tbnz x3, #0, 2f // goto store sp_el1 if bit[0] of spsr == 1

    // sp_el0
    mrs  x4, sp_el0
    b    3f // store

2:  // sp_el1
    add  x4, sp, #ECTX_SIZE
    
3:  // store sp_elx
    str  x4, [sp, #ECTX_OFFS_SP]


    // store vregs
    add x0, sp, #ECTX_OFFS_V

    st1 {v0.16b,  v1.16b},  [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v2.16b,  v3.16b},  [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v4.16b,  v5.16b},  [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v6.16b,  v7.16b},  [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v8.16b,  v9.16b},  [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v10.16b, v11.16b}, [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v12.16b, v13.16b}, [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v14.16b, v15.16b}, [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v16.16b, v17.16b}, [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v18.16b, v19.16b}, [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v20.16b, v21.16b}, [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v22.16b, v23.16b}, [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v24.16b, v25.16b}, [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v26.16b, v27.16b}, [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v28.16b, v29.16b}, [x0], #(2 * ECTX_SIZEOF_V)
    st1 {v30.16b, v31.16b}, [x0], #(2 * ECTX_SIZEOF_V)

    mov x0, sp
.endm


/// loads the ectx struct from the sp or from x0. The sp or x0 should point to the struct
.macro ectx_load reg_pt
.ifc \reg_pt, sp
.else
    .ifc \reg_pt, x0
    .else
        .error "reg_pt must be either sp or x0"
    .endif
.endif

    // load sysregs
    ldp x1, x2, [\reg_pt, #ECTX_OFFS_FPCR]
    msr fpcr, x1
    msr fpsr, x2

    ldp x3, x4, [\reg_pt, #ECTX_OFFS_ELR]
    msr elr_el1, x3
    msr spsr_el1, x4


    // load sp
    tbnz x4, #0, 0f
    

    ldr  x5,    [\reg_pt, #ECTX_OFFS_SP]
    msr  sp_el0, x5
    
.ifc \reg_pt, x0
    b    2f
0:
    ldr  x5,    [\reg_pt, #ECTX_OFFS_SP]
    mov  sp, x5
.else
    b    2f
0:
#   ifdef DEBUG
    //  if sp_el1 is selected, the stack should already be the correct stack when adding the size of the ectx struct size
    //  so just ignore it (in DEBUG assert it)  
    ldr  x5,    [\reg_pt, #ECTX_OFFS_SP]
    add  x6, sp, #ECTX_SIZE
    cmp  x5, x6
    b.eq 2f

1:  wfi
    b 1b
#   endif
.endif

2:  // load vregs
    add x1, \reg_pt, #ECTX_OFFS_V

    ld1 {v0.16b,  v1.16b},  [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v2.16b,  v3.16b},  [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v4.16b,  v5.16b},  [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v6.16b,  v7.16b},  [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v8.16b,  v9.16b},  [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v10.16b, v11.16b}, [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v12.16b, v13.16b}, [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v14.16b, v15.16b}, [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v16.16b, v17.16b}, [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v18.16b, v19.16b}, [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v20.16b, v21.16b}, [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v22.16b, v23.16b}, [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v24.16b, v25.16b}, [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v26.16b, v27.16b}, [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v28.16b, v29.16b}, [x1], #(2 * ECTX_SIZEOF_V)
    ld1 {v30.16b, v31.16b}, [x1], #(2 * ECTX_SIZEOF_V)


    // load xregs
    ldp x1, x2,     [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 1)]
    ldp x3, x4,     [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 3)]
    ldp x5, x6,     [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 5)]
    ldp x7, x8,     [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 7)]
    ldp x9, x10,    [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 9)]
    ldp x11, x12,   [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 11)]
    ldp x13, x14,   [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 13)]
    ldp x15, x16,   [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 15)]
    ldp x17, x18,   [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 17)]
    ldp x19, x20,   [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 19)]
    ldp x21, x22,   [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 21)]
    ldp x23, x24,   [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 23)]
    ldp x25, x26,   [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 25)]
    ldp x27, x28,   [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 27)]
    ldp x29, x30,   [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 29)]
    
    ldr x0,         [\reg_pt, #(ECTX_OFFS_X + ECTX_SIZEOF_X * 0)]

.ifc \reg_pt, sp 
    add sp, sp, #ECTX_SIZE
.endif
.endm

