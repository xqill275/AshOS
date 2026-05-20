.global gdt_flush

gdt_flush:
    /* get the pointer to the GDT descriptor from the stack */
    mov 4(%esp), %eax
    lgdt (%eax)

    /* reload the segment registers with the new data segment (0x10 = entry 2) */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* far jump to reload the code segment (0x08 = entry 1) */
    jmp $0x08, $.flush

.flush:
    ret
