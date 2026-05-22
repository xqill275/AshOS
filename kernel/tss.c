#include "../include/tss.h"
#include <stdint.h>

extern void kprintf(const char* fmt, ...);

/* defined in gdt.c */
extern void gdt_set_tss(uint32_t base, uint32_t limit);

static tss_t tss;

void tss_init(uint32_t kernel_stack)
{
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = base + sizeof(tss_t);

    /* zero out the TSS */
    uint8_t* p = (uint8_t*)&tss;
    for (uint32_t i = 0; i < sizeof(tss_t); i++)
        p[i] = 0;

    tss.ss0  = 0x10;           /* kernel data segment */
    tss.esp0 = kernel_stack;   /* kernel stack */

    /* add TSS to GDT and load it */
    gdt_set_tss(base, limit);

    /* load TSS — 0x28 is the TSS descriptor offset in our GDT */
    __asm__ volatile ("ltr %0" : : "r"((uint16_t)0x28));

    kprintf("TSS: initialised, kernel stack at %x\n", kernel_stack);
}

void tss_set_kernel_stack(uint32_t stack)
{
    tss.esp0 = stack;
}
