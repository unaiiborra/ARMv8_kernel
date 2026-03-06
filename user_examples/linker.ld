ENTRY(_start)

PHDRS
{
    text   PT_LOAD FILEHDR PHDRS FLAGS(5); /* R | X */
    rodata PT_LOAD FLAGS(4);                /* R     */
    data   PT_LOAD FLAGS(6);                /* R | W */
}

SECTIONS
{
    . = 0x400000;

    .text : ALIGN(4096)
    {
        *(.text*)
        . = ALIGN(4096);
    } :text

    .rodata : ALIGN(4096)
    {
        *(.rodata*)
        . = ALIGN(4096);
    } :rodata

    .data : ALIGN(4096)
    {
        *(.data*)
        . = ALIGN(4096);
    } :data

    .bss (NOLOAD) : ALIGN(4096)
    {
        *(.bss*)
        *(COMMON)
        . = ALIGN(4096);
    } :data
}