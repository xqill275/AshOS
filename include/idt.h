#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* Each IDT entry is 8 bytes */
typedef struct {
    uint16_t base_low;     /* lower 16 bits of handler address */
    uint16_t selector;     /* kernel code segment selector (0x08) */
    uint8_t  zero;         /* always zero */
    uint8_t  flags;        /* type and attributes */
    uint16_t base_high;    /* upper 16 bits of handler address */
} __attribute__((packed)) idt_entry_t;

/* Tells the CPU where the IDT is and how big it is */
typedef struct {
    uint16_t limit;        /* size of IDT minus 1 */
    uint32_t base;         /* address of first entry */
} __attribute__((packed)) idt_descriptor_t;

/* Registers pushed by our assembly handlers */
typedef struct {
    uint32_t ds;                            /* data segment */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pushed by pusha */
    uint32_t int_no, err_code;              /* interrupt number and error code */
    uint32_t eip, cs, eflags, useresp, ss; /* pushed by the CPU automatically */
} registers_t;

void idt_init(void);

#endif
