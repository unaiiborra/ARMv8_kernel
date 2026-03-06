#pragma once

typedef const unsigned long embedded_elf_size;
typedef const unsigned char embedded_elf;

extern embedded_elf_size SYSC_PRINT_ELF_SIZE;
extern embedded_elf SYSC_PRINT_ELF[];

extern embedded_elf_size SVC_ELF_SIZE;
extern embedded_elf SVC_ELF[];

extern embedded_elf_size TEST_ELF_SIZE;
extern embedded_elf TEST_ELF[];
