ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5); /* R + X */
}

SECTIONS {
    . = 0x400000;          /* VA userspace de prueba */

    .text : {
        *(.text*)
        . = ALIGN(4096);
    } :text
}