#include "../include/gdt.h"

static gdt_entry_t gdt[3];
static gdt_descriptor_t gdt_descriptor;

extern void gdt_flush(uint32_t);

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

void gdt_init(void)
{
    gdt_descriptor.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdt_descriptor.base  = (uint32_t)&gdt;

    gdt_set_entry(0, 0, 0x00000000, 0x00, 0x00); /* null descriptor */
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /* kernel code    */
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); /* kernel data    */

    gdt_flush((uint32_t)&gdt_descriptor);
}
