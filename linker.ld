OUTPUT_FORMAT("elf64-littleaarch64")
ENTRY(_start)


INCLUDE memory.ld

KERNEL_PA_BASE = ORIGIN(DDR) + __TFA_PROTECTED_SIZE;
KERNEL_VA_BASE = KERNEL_PA_BASE + HI_VA_BASE;

SECTIONS {
    . = KERNEL_VA_BASE;

    .text KERNEL_VA_BASE : ALIGN(4096) {
        __text_start = .;
        KEEP(*(.text.boot))
        *(.text*)

        __vectors_start = .;
        KEEP(*(.vectors))
        __vectors_end = .;

        . = ALIGN(4096);
        __text_end = .;
    } > HI_VMEM AT > DDR_KERNEL
    
    __text_size = __text_end - __text_start;


    .rodata : ALIGN(4096) {
        __rodata_start = .;
        *(.rodata*)
        *(.rodata.*)
        *(.platform_info)

       
        . = ALIGN(8);
        __kernel_initcalls_start = .;
        KEEP(*(.kernel_initcall))
        __kernel_initcalls_end = .;


        . = ALIGN(4096);
        __rodata_end = .;
    } > HI_VMEM AT > DDR_KERNEL

    __rodata_size = __rodata_end - __rodata_start;


    .data : ALIGN(4096) {
        __data_start = .;
        *(.data*)
        

        . = ALIGN(4096);
        __data_end = .;
    } > HI_VMEM AT > DDR_KERNEL

    __data_size = __data_end - __data_start;


    .bss : ALIGN(4096) {
        __bss_start = .;
        *(.bss*)
        *(COMMON)


        . = ALIGN(4096);
        __bss_end = .;
    } > HI_VMEM AT > DDR_KERNEL

    __bss_size = __bss_end - __bss_start;

    .stacks (NOLOAD) : ALIGN(4096) {
        __stacks_start = .;

        __stacks_el2_start = .;
        . += (__NUM_CORES * __EL2_STACK_SIZE);
        __stacks_el2_end = .;
        . = ALIGN(16);
        __stacks_el1_start = .;
        . += (__NUM_CORES * __EL1_STACK_SIZE);
        __stacks_el1_end = .;

        . = ALIGN(4096);
        __stacks_end = .;
    } > HI_VMEM AT > DDR_KERNEL

    __stacks_size = __stacks_end - __stacks_start;



    __heap_start = .;
    __heap_end = ~0;
    __heap_size = __heap_end - __heap_start;
}
