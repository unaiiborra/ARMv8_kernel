.section .text
.align 4



.global _jmp_to_with_offset
// void _jmp_to_with_offset(puintptr_t to, size_t offset, uint64_t ctx)
_jmp_to_with_offset:
    add x3, x0, x1  // to += offset
    mov x0, x2
    br  x3

.global _reloc_cfg_end
_reloc_cfg_end:
    msr daifset, #0xF
    isb

    mov x15, x0

    mov x0, sp      //  (puintptr_t pa)
    bl _mm_kpa_to_kva    //  return pa + KERNEL_BASE
    mov sp, x0      //  sp = remap res


    mrs x0, VBAR_EL1
    bl _mm_kpa_to_kva
    msr VBAR_EL1, x0

    mov lr, x15
    isb
    ret


// noreturn void _relocated_return(
//      x0:   vuintptr_t ret_addr
// );
.global _reset_stack_and_return
_reset_stack_and_return:
    mov x20, x0
    bl  _reset_stack_el1

    blr x20

1:  wfi
    b   1b

