#ifndef VMM_H
#define VMM_H

#include <stdint.h>

/* page directory/table entry flags */
#define PAGE_PRESENT    0x1   /* page is present in memory */
#define PAGE_WRITABLE   0x2   /* page is writable */
#define PAGE_USER       0x4   /* page is accessible from userspace */

void vmm_init(void);
void vmm_map(uint32_t virt, uint32_t phys, uint32_t flags);

#endif
