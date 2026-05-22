#include "../include/gdt.h"

static gdt_entry_t gdt[6];
static gdt_descriptor_t gdt_descriptor;

extern void gdt_flush(uint32_t);
extern void tss_flush(void);

static void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t granularity)
{
    gdt[index].base_low    = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high   = (base >> 24) & 0xFF;
    gdt[index].limit_low   = (limit & 0xFFFF);
    gdt[index].granularity = (limit >> 16) & 0x0F;
    gdt[index].granularity |= granularity & 0xF0;
    gdt[index].access      = access;
}

void gdt_set_tss(uint32_t base, uint32_t limit)
{
    gdt_set_entry(5, base, limit, 0x89, 0x00);
}

void gdt_init(void)
{
    gdt_descriptor.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_descriptor.base  = (uint32_t)&gdt;

    gdt_set_entry(0, 0, 0x00000000, 0x00, 0x00); /* null          */
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /* kernel code   */
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); /* kernel data   */
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); /* user code     */
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); /* user data     */
    gdt_set_entry(5, 0, 0x00000000, 0x00, 0x00); /* TSS (filled later) */

    gdt_flush((uint32_t)&gdt_descriptor);
}
