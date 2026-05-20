#include "../include/pmm.h"
#include <stdint.h>
#include <stddef.h>

extern void kprintf(const char* fmt, ...);

/* kernel_end is defined in linker.ld */
extern uint32_t kernel_end;

#define MAX_PAGES     32768        /* supports up to 128MB RAM */
#define BITMAP_SIZE   (MAX_PAGES / 32)

static uint32_t bitmap[BITMAP_SIZE];
static uint32_t total_pages = 0;
static uint32_t used_pages  = 0;

/* ============================================================
   Bitmap helpers
   ============================================================ */

static void bitmap_set(uint32_t page)
{
    bitmap[page / 32] |= (1 << (page % 32));
}

static void bitmap_clear(uint32_t page)
{
    bitmap[page / 32] &= ~(1 << (page % 32));
}

static uint32_t bitmap_test(uint32_t page)
{
    return bitmap[page / 32] & (1 << (page % 32));
}

/* ============================================================
   Init
   ============================================================ */

void pmm_init(uint32_t mem_upper)
{
    /*
     * mem_upper is in KB (from multiboot), convert to pages.
     * Add 1MB for lower memory to get total RAM in pages.
     */
    total_pages = (mem_upper + 1024) / 4;
    if (total_pages > MAX_PAGES)
        total_pages = MAX_PAGES;

    /* mark everything as used to start */
    for (uint32_t i = 0; i < BITMAP_SIZE; i++)
        bitmap[i] = 0xFFFFFFFF;

    /*
     * Free pages above the kernel end.
     * kernel_end is a physical address set by the linker.
     * We round up to the next page boundary.
     */
    uint32_t kernel_end_page = ((uint32_t)&kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = kernel_end_page; i < total_pages; i++) {
        bitmap_clear(i);
        used_pages--;
    }

    used_pages = kernel_end_page;

    kprintf("PMM: %d pages total, %d used by kernel, %d free\n",
            total_pages,
            used_pages,
            total_pages - used_pages);
}

/* ============================================================
   Alloc / Free
   ============================================================ */

void* pmm_alloc(void)
{
    for (uint32_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_pages++;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return 0; /* out of memory */
}

void pmm_free(void* addr)
{
    uint32_t page = (uint32_t)addr / PAGE_SIZE;
    bitmap_clear(page);
    used_pages--;
}

uint32_t pmm_free_pages(void)
{
    return total_pages - used_pages;
}
