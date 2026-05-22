#include <stdint.h>

extern void kprintf(const char* fmt, ...);

/*
 * Switch to ring 3 and jump to the given function.
 * This is a one way trip — we don't return from here.
 *
 * The iret instruction pops in order:
 * eip, cs, eflags, esp, ss
 * We push these manually to fake a return from an interrupt
 * into user mode.
 */
void enter_usermode(uint32_t entry, uint32_t user_stack)
{
    kprintf("Entering ring 3 at 0x%x\n", entry);

    __asm__ volatile (
        "cli\n"

        /* set data segments to user data selector */
        /* 0x23 = GDT entry 4 (user data) with RPL=3 */
        "mov $0x23, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"

        /* push ss, esp, eflags, cs, eip for iret */
        "push $0x23\n"          /* ss  - user data segment */
        "push %1\n"             /* esp - user stack */
        "pushf\n"               /* eflags */
        "pop %%eax\n"
        "or $0x200, %%eax\n"    /* set interrupt enable flag */
        "push %%eax\n"
        "push $0x1B\n"          /* cs  - user code segment (GDT entry 3, RPL=3) */
        "push %0\n"             /* eip - entry point */
        "iret\n"
        : : "r"(entry), "r"(user_stack) : "eax"
    );
}
