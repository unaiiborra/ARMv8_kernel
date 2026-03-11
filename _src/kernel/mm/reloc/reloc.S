.section .text
.align 4



.global _jmp_to_with_offset
// _Noreturn void _jmp_to_with_offset(void* to, size_t offset);
_jmp_to_with_offset:
    add x0, x0, x1  // to += offset
    br  x0


.global _reloc_cfg_end
_reloc_cfg_end:
    msr daifset, #0xF
    isb


    mov x0, sp      //  (p_uintptr_t pa)
    bl _mm_kpa_to_kva    //  return pa + KERNEL_BASE
    mov sp, x0      //  sp = remap res


    mrs x0, VBAR_EL1
    bl _mm_kpa_to_kva
    msr VBAR_EL1, x0

    adr lr, reloc_cfg_end // lr to kernel_entry remapped

    isb
    ret


// returns to the start of kernel entry after fully resetting the sp
// void _return_to_kernel_entry(size_t physmap_offset);
.global _return_to_kernel_entry
_return_to_kernel_entry:
    bl _reset_stack_el1

    adr lr, kernel_entry
    ret

