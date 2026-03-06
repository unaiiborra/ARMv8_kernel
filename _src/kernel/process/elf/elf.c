#include "elf.h"

#include <arm/mmu.h>
#include <kernel/mm.h>
#include <kernel/mm/vmalloc.h>
#include <kernel/panic.h>
#include <kernel/process/process.h>
#include <lib/mem.h>
#include <lib/stdint.h>

#include "../process/process.h"
#include "lib/align.h"


#define EI_NIDENT 16

typedef struct {
    uint8 e_ident[EI_NIDENT]; /* 0x7F 'E' 'L' 'F' */
    uint16 eype;              /* ET_EXEC, ET_DYN */
    uint16 e_machine;         /* EM_AARCH64 */
    uint32 e_version;
    uint64 e_entry; /* entry point */
    uint64 e_phoff; /* offset PHDR table */
    uint64 e_shoff; /* offset SHDR table (ignorar) */
    uint32 e_flags;
    uint16 e_ehsize;    /* sizeof(Elf64_Ehdr) */
    uint16 e_phentsize; /* sizeof(Elf64_Phdr) */
    uint16 e_phnum;     /* nº program headers */
    uint16 e_shentsize;
    uint16 e_shnum;
    uint16 e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32 p_type;   /* PT_LOAD */
    uint32 p_flags;  /* PF_X | PF_W | PF_R */
    uint64 p_offset; /* offset en archivo */
    uint64 p_vaddr;  /* VA destino */
    uint64 p_paddr;
    uint64 p_filesz; /* bytes a copiar */
    uint64 p_memsz;  /* bytes en memoria */
    uint64 p_align;
} Elf64_Phdr;


typedef enum {
    PT_NULL = 0,
    PT_LOAD = 1,
    PT_DYNAMIC = 2,
    PT_INTERP = 3,
    PT_NOTE = 4,
    PT_SHLIB = 5,
    PT_PHDR = 6,
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7fffffff,
} segtype;


// elf p_flags bits
#define PF_X_BIT 0 // execute
#define PF_W_BIT 1 // write
#define PF_R_BIT 2 // read
#define PF_X (1ULL << PF_X_BIT)
#define PF_W (1ULL << PF_W_BIT)
#define PF_R (1ULL << PF_R_BIT)


extern void _cache_flush_range(void* start, void* end);

/// read-write
static const mmu_pg_cfg USR_MMU_RW_CFG = (mmu_pg_cfg) {
    .attr_index = 0,
    .ap = MMU_AP_EL0_RW_EL1_RW,
    .shareability = MMU_SH_INNER_SHAREABLE,
    .non_secure = false,
    .access_flag = 1,
    .pxn = true,
    .uxn = true,
    .sw = 0,
};

/// read-only
static const mmu_pg_cfg USR_MMU_RO_CFG = (mmu_pg_cfg) {
    .attr_index = 0,
    .ap = MMU_AP_EL0_RO_EL1_RO,
    .shareability = MMU_SH_INNER_SHAREABLE,
    .non_secure = false,
    .access_flag = 1,
    .pxn = true,
    .uxn = true,
    .sw = 0,
};

/// read-executable
static const mmu_pg_cfg USR_MMU_RX_CFG = (mmu_pg_cfg) {
    .attr_index = 0,
    .ap = MMU_AP_EL0_RO_EL1_RO,
    .shareability = MMU_SH_INNER_SHAREABLE,
    .non_secure = false,
    .access_flag = 1,
    .pxn = true,
    .uxn = false,
    .sw = 0,
};

elf_load_result elf_load(void* elf, size_t size, proc* usr_proc)
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
    size_t ph_n;

    ph = (Elf64_Phdr*)((v_uintptr)elf + (v_uintptr)eh->e_phoff);
    ph_n = eh->e_phnum;


    for (size_t i = 0; i < ph_n; i++) {
        if (ph[i].p_type != PT_LOAD || ph[i].p_memsz == 0)
            continue;

        ASSERT(ph[i].p_offset + ph[i].p_filesz <= size);
        ASSERT(ph[i].p_memsz % KPAGE_SIZE == 0);
        ASSERT(ph[i].p_vaddr % KPAGE_SIZE == 0);
        ASSERT(ph[i].p_align % KPAGE_SIZE == 0);

        ASSERT(ph[i].p_flags & PF_R, "elf section marked as read disabled");

        uint64 data_flags = ph[i].p_flags & (PF_R | PF_W | PF_X);

        const mmu_pg_cfg* MMU_CFG;
        switch (data_flags) {
            case PF_R | PF_W:
                MMU_CFG = &USR_MMU_RW_CFG; // read-write
                break;
            case PF_R:
                MMU_CFG = &USR_MMU_RO_CFG; // read-only
                break;
            case PF_R | PF_X:
                MMU_CFG = &USR_MMU_RX_CFG; // read-executable
                break;
            case PF_R | PF_W | PF_X:
                PANIC("elf region configured as executable and writable");
            default:
                PANIC();
        }

        void* kva = process_malloc_usr_region(
            usr_proc,
            ph[i].p_vaddr,
            ph[i].p_memsz / KPAGE_SIZE,
            MMU_CFG);


        if (ph[i].p_filesz != 0)
            memcpy(kva, (uint8*)elf + ph[i].p_offset, ph[i].p_filesz);


        // memzero the remaining area
        if (ph[i].p_memsz > ph[i].p_filesz) {
            void* dst = (void*)((uintptr)kva + ph[i].p_filesz);
            size_t rem_size = ph[i].p_memsz - ph[i].p_filesz;

            memzero(dst, rem_size);
        }

        _cache_flush_range(
            align_down_ptr(kva, 64),
            align_down_ptr((void*)((uintptr)kva + ph[i].p_memsz), 64));
    }

    usr_proc->entry = eh->e_entry;

    return ELF_LOAD_OK;
}
