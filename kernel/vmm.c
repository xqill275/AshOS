#include "../include/vmm.h"
#include "../include/pmm.h"
#include <stdint.h>

extern void kprintf(const char* fmt, ...);

/*
 * The page directory — 1024 entries, each 4 bytes.
 * Must be page aligned (4KB boundary).
 * Each entry points to a page table.
 */
static uint32_t page_directory[1024] __attribute__((aligned(4096)));

/*
 * We pre-allocate page tables for the first 4MB (2 tables).
 * This covers the kernel which loads at 2MB.
 */
static uint32_t page_tables[2][1024] __attribute__((aligned(4096)));

/* ============================================================
   Internal helpers
   ============================================================ */

/*
 * Get or create a page table for a given virtual address.
 * Returns a pointer to the page table.
 */
static uint32_t* vmm_get_table(uint32_t virt, uint32_t flags)
{
    uint32_t dir_index = virt >> 22; /* top 10 bits */

    if (page_directory[dir_index] & PAGE_PRESENT) {
        /* table already exists, return it */
        return (uint32_t*)(page_directory[dir_index] & ~0xFFF);
    }

    /* allocate a new page table */
    uint32_t* table = (uint32_t*)pmm_alloc();
    if (!table) return 0;

    /* zero it out */
    for (int i = 0; i < 1024; i++)
        table[i] = 0;

    /* install it in the page directory */
    page_directory[dir_index] = (uint32_t)table | flags | PAGE_PRESENT;
    return table;
}

/* ============================================================
   Public API
   ============================================================ */

/*
 * Map a virtual address to a physical address.
 */
void vmm_map(uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t tbl_index = (virt >> 12) & 0x3FF; /* next 10 bits */

    uint32_t* table = vmm_get_table(virt, flags);
    if (!table) {
        kprintf("VMM: failed to get page table for 0x%x\n", virt);
        return;
    }

    table[tbl_index] = (phys & ~0xFFF) | flags | PAGE_PRESENT;
}

/*
 * Initialise virtual memory.
 * Identity maps the first 4MB so the kernel keeps working
 * after paging is enabled.
 */
void vmm_init(void)
{
    /* clear the page directory */
    for (int i = 0; i < 1024; i++)
        page_directory[i] = 0;

    /*
     * Identity map the first 4MB using our pre-allocated tables.
     * virtual address == physical address for 0x0 to 0x400000.
     * This keeps the kernel working after paging is enabled.
     */
    page_directory[0] = (uint32_t)page_tables[0] | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    page_directory[1] = (uint32_t)page_tables[1] | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    for (int i = 0; i < 1024; i++) {
        page_tables[0][i] = (i * 4096) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        page_tables[1][i] = ((1024 + i) * 4096) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }

    /*
     * Load the page directory address into CR3.
     * Then enable paging by setting bit 31 of CR0.
     */
    __asm__ volatile (
        "mov %0, %%cr3\n"         /* load page directory */
        "mov %%cr0, %%eax\n"      /* read CR0 */
        "or $0x80000000, %%eax\n" /* set paging bit */
        "mov %%eax, %%cr0\n"      /* write CR0 - paging enabled */
        : : "r"(page_directory) : "eax"
    );

    kprintf("VMM: paging enabled, first 4MB identity mapped\n");
}
