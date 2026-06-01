#include "../include/elf.h"
#include "../include/fs.h"
#include "../include/heap.h"
#include "../include/vmm.h"
#include "../include/pmm.h"
#include <stdint.h>

extern void kprintf(const char* fmt, ...);

static void elf_memcpy(void* dst, const void* src, uint32_t size)
{
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < size; i++)
        d[i] = s[i];
}

static void elf_memset(void* ptr, uint8_t val, uint32_t size)
{
    uint8_t* p = (uint8_t*)ptr;
    for (uint32_t i = 0; i < size; i++)
        p[i] = val;
}

/*
 * Load an ELF file from AshFS into memory.
 * Returns 0 on success, -1 on failure.
 * Sets entry_point to the ELF entry address.
 */
int elf_load(const char* filename, uint32_t* entry_point)
{
    /* read the file from AshFS into a buffer */
    uint8_t* buf = (uint8_t*)kmalloc(8192);
    if (!buf) {
        kprintf("ELF: out of memory\n");
        return -1;
    }

    int bytes = fs_read(filename, buf, 8192);
    if (bytes <= 0) {
        kprintf("ELF: could not read '%s'\n", filename);
        kfree(buf);
        return -1;
    }

    /* check ELF magic number */
    elf_header_t* header = (elf_header_t*)buf;
    if (header->magic != ELF_MAGIC) {
        kprintf("ELF: invalid magic number\n");
        kfree(buf);
        return -1;
    }

    /* check it's a 32-bit x86 executable */
    if (header->bits != 1 || header->machine != EM_386) {
        kprintf("ELF: not a 32-bit x86 binary\n");
        kfree(buf);
        return -1;
    }

    if (header->type != ET_EXEC) {
        kprintf("ELF: not an executable\n");
        kfree(buf);
        return -1;
    }

    kprintf("ELF: loading '%s', entry at %x\n", filename, header->entry);

    /* process program headers */
    elf_program_header_t* ph = (elf_program_header_t*)(buf + header->ph_offset);

    for (uint16_t i = 0; i < header->ph_count; i++) {
        if (ph[i].type != PT_LOAD) continue;

        kprintf("ELF: segment %d: vaddr=%x size=%d\n",
                i, ph[i].vaddr, ph[i].mem_size);

        /* allocate and map pages for this segment */
        uint32_t pages = (ph[i].mem_size + 4095) / 4096;
        for (uint32_t p = 0; p < pages; p++) {
            uint32_t phys = (uint32_t)pmm_alloc();
            if (!phys) {
                kprintf("ELF: out of physical memory\n");
                kfree(buf);
                return -1;
            }
            vmm_map(ph[i].vaddr + p * 4096, phys,
                    PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

            /* flush TLB */
            __asm__ volatile (
                "mov %%cr3, %%eax\n"
                "mov %%eax, %%cr3\n"
                : : : "eax"
            );
        }

        /* copy segment data from file */
        elf_memcpy((void*)ph[i].vaddr, buf + ph[i].offset, ph[i].file_size);

        /* zero out any extra memory (bss) */
        if (ph[i].mem_size > ph[i].file_size) {
            elf_memset((void*)(ph[i].vaddr + ph[i].file_size),
                       0, ph[i].mem_size - ph[i].file_size);
        }
    }

    *entry_point = header->entry;
    kfree(buf);
    kprintf("ELF: loaded successfully\n");
    return 0;
}
