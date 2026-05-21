#include "../include/heap.h"
#include "../include/pmm.h"
#include "../include/vmm.h"
#include <stdint.h>
#include <stddef.h>

extern void kprintf(const char* fmt, ...);
extern uint32_t kernel_end;

/*
 * Block header — stored just before every allocation.
 * The 'next' pointer links all blocks together.
 */
typedef struct block {
    size_t        size;    /* size of usable memory (not including header) */
    uint8_t       free;    /* 1 = free, 0 = used */
    struct block* next;    /* next block in the list */
} block_t;

#define HEAP_START  0x01000000  /* 16MB virtual address - safely above kernel */
#define HEAP_SIZE   0x00100000  /* 1MB initial heap */
#define BLOCK_SIZE  sizeof(block_t)

static block_t* heap_head = 0;

/* ============================================================
   Internal helpers
   ============================================================ */

/*
 * Map physical pages to cover the heap virtual address range.
 */
static void heap_map_pages(uint32_t virt_start, uint32_t size)
{
    uint32_t pages = (size + 4095) / 4096;
    for (uint32_t i = 0; i < pages; i++) {
        uint32_t phys = (uint32_t)pmm_alloc();
        if (!phys) {
            kprintf("heap: out of physical memory!\n");
            return;
        }
        vmm_map(virt_start + i * 4096, phys, 0x3); /* present + writable */
    }
}

/*
 * Merge adjacent free blocks to reduce fragmentation.
 */
static void heap_coalesce(void)
{
    block_t* curr = heap_head;
    while (curr && curr->next) {
        if (curr->free && curr->next->free) {
            curr->size += BLOCK_SIZE + curr->next->size;
            curr->next  = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

/* ============================================================
   Public API
   ============================================================ */

void heap_init(void)
{
    /* map physical pages for the heap */
    heap_map_pages(HEAP_START, HEAP_SIZE);

    /* set up the initial free block covering the whole heap */
    heap_head        = (block_t*)HEAP_START;
    heap_head->size  = HEAP_SIZE - BLOCK_SIZE;
    heap_head->free  = 1;
    heap_head->next  = 0;

    kprintf("Heap: initialised at %x, size %d KB\n",
            HEAP_START, HEAP_SIZE / 1024);
}

void* kmalloc(size_t size)
{
    if (size == 0) return 0;

    /* align size to 4 bytes */
    size = (size + 3) & ~3;

    block_t* curr = heap_head;

    while (curr) {
        if (curr->free && curr->size >= size) {
            /*
             * Split the block if there's enough space left over
             * for a new block header plus at least 4 bytes.
             */
            if (curr->size >= size + BLOCK_SIZE + 4) {
                block_t* new_block  = (block_t*)((uint8_t*)curr + BLOCK_SIZE + size);
                new_block->size     = curr->size - size - BLOCK_SIZE;
                new_block->free     = 1;
                new_block->next     = curr->next;
                curr->next          = new_block;
                curr->size          = size;
            }

            curr->free = 0;
            return (void*)((uint8_t*)curr + BLOCK_SIZE);
        }
        curr = curr->next;
    }

    kprintf("kmalloc: out of heap memory!\n");
    return 0;
}

void kfree(void* ptr)
{
    if (!ptr) return;

    block_t* block = (block_t*)((uint8_t*)ptr - BLOCK_SIZE);
    block->free = 1;

    /* merge adjacent free blocks */
    heap_coalesce();
}
