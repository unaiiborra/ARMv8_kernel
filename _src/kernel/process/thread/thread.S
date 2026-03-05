.section .text

/*
    void _thread_switch_el0(
    x0    v_uintptr pc,
    x1    uint64 fpcr,
    x2    uint64 fpsr,
    x3    uint64 sp,
    x4    uint64 gpr[31],
    x5    uint64 vec[32][2]
    );
*/
.global _thread_switch_el0
_thread_switch_el0:
    msr ELR_EL1, x0
    msr FPCR, x1
    msr FPSR, x2
    msr SP_EL0, x3
    
    mov x6, #0
    msr SPSR_EL1, x6

    // load vec registers
    ld1 {v0.16b, v1.16b}, [x5], #32
    ld1 {v2.16b, v3.16b}, [x5], #32
    ld1 {v4.16b, v5.16b}, [x5], #32
    ld1 {v6.16b, v7.16b}, [x5], #32
    ld1 {v8.16b, v9.16b}, [x5], #32
    ld1 {v10.16b, v11.16b}, [x5], #32
    ld1 {v12.16b, v13.16b}, [x5], #32
    ld1 {v14.16b, v15.16b}, [x5], #32
    ld1 {v16.16b, v17.16b}, [x5], #32
    ld1 {v18.16b, v19.16b}, [x5], #32
    ld1 {v20.16b, v21.16b}, [x5], #32
    ld1 {v22.16b, v23.16b}, [x5], #32
    ld1 {v24.16b, v25.16b}, [x5], #32
    ld1 {v26.16b, v27.16b}, [x5], #32
    ld1 {v28.16b, v29.16b}, [x5], #32
    ld1 {v30.16b, v31.16b}, [x5], #32

    // load gpr
    mov x0, x4

    ldp x2,  x3,  [x0, #(16 * 1)]
    ldp x4,  x5,  [x0, #(16 * 2)]
    ldp x6,  x7,  [x0, #(16 * 3)]
    ldp x8,  x9,  [x0, #(16 * 4)]
    ldp x10, x11, [x0, #(16 * 5)]
    ldp x12, x13, [x0, #(16 * 6)]
    ldp x14, x15, [x0, #(16 * 7)]
    ldp x16, x17, [x0, #(16 * 8)]
    ldp x18, x19, [x0, #(16 * 9)]
    ldp x20, x21, [x0, #(16 * 10)]
    ldp x22, x23, [x0, #(16 * 11)]
    ldp x24, x25, [x0, #(16 * 12)]
    ldp x26, x27, [x0, #(16 * 13)]
    ldp x28, x29, [x0, #(16 * 14)]
    ldr x30, [x0, #(16 * 15)]
    
    ldp x0, x1, [x0]

    eret
    ret
