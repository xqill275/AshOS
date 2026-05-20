#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

void  pmm_init(uint32_t mem_upper);
void* pmm_alloc(void);
void  pmm_free(void* addr);
uint32_t pmm_free_pages(void);

#endif
