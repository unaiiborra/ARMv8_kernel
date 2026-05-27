.section .text


.global _init_gpr
_init_gpr:
    str x30, [sp, #-16]!
    
    mov x0, xzr
    mov x1, xzr
    mov x2, xzr
    mov x3, xzr
    mov x4, xzr
    mov x5, xzr
    mov x6, xzr
    mov x7, xzr
    mov x8, xzr
    mov x9, xzr
    mov x10, xzr
    mov x11, xzr
    mov x12, xzr
    mov x13, xzr
    mov x14, xzr
    mov x15, xzr
    mov x16, xzr
    mov x17, xzr
    mov x18, xzr
    mov x19, xzr
    mov x20, xzr
    mov x21, xzr
    mov x22, xzr
    mov x23, xzr
    mov x24, xzr
    mov x25, xzr
    mov x26, xzr
    mov x27, xzr
    mov x28, xzr
    mov x29, xzr
    mov x30, xzr

    movi  v0.16b,  #0
    movi  v1.16b,  #0
    movi  v2.16b,  #0
    movi  v3.16b,  #0
    movi  v4.16b,  #0
    movi  v5.16b,  #0
    movi  v6.16b,  #0
    movi  v7.16b,  #0
    movi  v8.16b,  #0
    movi  v9.16b,  #0
    movi  v10.16b, #0
    movi  v11.16b, #0
    movi  v12.16b, #0
    movi  v13.16b, #0
    movi  v14.16b, #0
    movi  v15.16b, #0
    movi  v16.16b, #0
    movi  v17.16b, #0
    movi  v18.16b, #0
    movi  v19.16b, #0
    movi  v20.16b, #0
    movi  v21.16b, #0
    movi  v22.16b, #0
    movi  v23.16b, #0
    movi  v24.16b, #0
    movi  v25.16b, #0
    movi  v26.16b, #0
    movi  v27.16b, #0
    movi  v28.16b, #0
    movi  v29.16b, #0
    movi  v30.16b, #0
    movi  v31.16b, #0

    ldr x30, [sp], #16

    ret