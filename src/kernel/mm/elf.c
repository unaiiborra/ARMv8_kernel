#include <kernel/mm/elf.h>
#include <kernel/mm/uregion.h>
#include <stdint.h>

#include "kernel/panic.h"
#include "lib/align.h"


#define EI_NIDENT 16

typedef struct {
    uint8_t  e_ident[EI_NIDENT]; /* 0x7F 'E' 'L' 'F' */
    uint16_t eype;               /* ET_EXEC, ET_DYN */
    uint16_t e_machine;          /* EM_AARCH64 */
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;


typedef struct {
    uint32_t p_type;  /* PT_LOAD */
    uint32_t p_flags; /* PF_X | PF_W | PF_R */
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;


typedef enum {
    PT_NULL    = 0,
    PT_LOAD    = 1,
    PT_DYNAMIC = 2,
    PT_INTERP  = 3,
    PT_NOTE    = 4,
    PT_SHLIB   = 5,
    PT_PHDR    = 6,
    PT_LOPROC  = 0x70000000,
    PT_HIPROC  = 0x7fffffff,
} segtype;


#define PF_X_BIT 0 // execute
#define PF_W_BIT 1 // write
#define PF_R_BIT 2 // read
#define PF_X     (1ULL << PF_X_BIT)
#define PF_W     (1ULL << PF_W_BIT)
#define PF_R     (1ULL << PF_R_BIT)


elf_load_result elf_load(task_t* t, void* elf, size_t size, uintptr_t* out_entry)
{
    spinlocked(&t->memory_lock)
    {
        ASSERT(elf);

        Elf64_Ehdr* eh = (Elf64_Ehdr*)elf;

        if (eh->e_ident[0] != 0x7F)
            return ELF_LOAD_BAD_MAGIC;
        if (eh->e_ident[1] != 'E')
            return ELF_LOAD_BAD_HDR;
        if (eh->e_ident[2] != 'L')
            return ELF_LOAD_BAD_HDR;
        if (eh->e_ident[3] != 'F')
            return ELF_LOAD_BAD_HDR;

        if (eh->e_ident[4] != 2)
            return ELF_LOAD_BAD_HDR; /* ELFCLASS64 */
        if (eh->e_ident[5] != 1)
            return ELF_LOAD_BAD_HDR; /* little endian */
        if (eh->e_machine != 183)
            return ELF_LOAD_BAD_HDR; /* EM_AARCH64 */


        Elf64_Phdr* ph;
        size_t      ph_n;

        ph   = (Elf64_Phdr*)((vuintptr_t)elf + (vuintptr_t)eh->e_phoff);
        ph_n = eh->e_phnum;


        for (size_t i = 0; i < ph_n; i++) {
            if (ph[i].p_type != PT_LOAD || ph[i].p_memsz == 0)
                continue;

            ASSERT(ph[i].p_offset + ph[i].p_filesz <= size);
            ASSERT(ph[i].p_vaddr % PAGE_SIZE == 0);
            ASSERT(ph[i].p_align % PAGE_SIZE == 0);

            ASSERT(ph[i].p_flags & PF_R, "elf section marked as read disabled");

            uint64_t memsz = align_up(ph[i].p_memsz, PAGE_SIZE);

            uint64_t data_flags = ph[i].p_flags & (PF_R | PF_W | PF_X);

            uregion_reserve_e ureg = uregion_reserve_static(
                t,
                ph[i].p_vaddr,
                memsz / PAGE_SIZE,
                data_flags & PF_R,
                data_flags & PF_W,
                data_flags & PF_X,
                false);

            ASSERT(ureg == UREGION_OK);

            if (ph[i].p_filesz != 0) {
                uregion_access_e uaccess = umemcpy(
                    t,
                    (void*)ph[i].p_vaddr,
                    (uint8_t*)elf + ph[i].p_offset,
                    ph[i].p_filesz,
                    UREGION_REQUIRED_FLAGS_IGNORE,
                    UREGION_FORBIDDEN_FLAGS_IGNORE,
                    false,
                    UMEMCPY_KNL_TO_USR);

                ASSERT(uaccess == UREGION_ACCESS_OK);
            }

            // memzero the remaining area
            if (memsz > ph[i].p_filesz) {
                uregion_access_e uaccess = umemzero(
                    t,
                    (void*)(ph[i].p_vaddr + ph[i].p_filesz),
                    memsz - ph[i].p_filesz,
                    UREGION_REQUIRED_FLAGS_IGNORE,
                    UREGION_FORBIDDEN_FLAGS_IGNORE,
                    false);

                ASSERT(uaccess == UREGION_ACCESS_OK);
            }

            if (data_flags & PF_X)
                uregion_flush_icache(t, ph[i].p_vaddr, memsz);
        }

        if (out_entry)
            *out_entry = eh->e_entry;
    }

    return ELF_LOAD_OK;
}
