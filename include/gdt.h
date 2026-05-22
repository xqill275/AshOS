#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* Each GDT entry is 8 bytes */
typedef struct {
    uint16_t limit_low;    /* lower 16 bits of limit */
    uint16_t base_low;     /* lower 16 bits of base */
    uint8_t  base_middle;  /* next 8 bits of base */
    uint8_t  access;       /* access byte */
    uint8_t  granularity;  /* flags + upper 4 bits of limit */
    uint8_t  base_high;    /* last 8 bits of base */
} __attribute__((packed)) gdt_entry_t;

/* Tells the CPU where the GDT is and how big it is */
typedef struct {
    uint16_t limit;        /* size of GDT minus 1 */
    uint32_t base;         /* address of first entry */
} __attribute__((packed)) gdt_descriptor_t;


void gdt_set_tss(uint32_t base, uint32_t limit);
void gdt_init(void);

#endif
