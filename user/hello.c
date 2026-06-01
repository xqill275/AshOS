#include "libash.h"

__attribute__((naked, section(".text._start"))) void _start(void)
{
    __asm__ volatile (
        "mov $0x500000, %esp\n"
        "call hello_main\n"
        "ud2\n"
    );
}

void hello_main(void)
{
    println("Hello from ELF!");
    println("Using libash!");
    sys_exit(0);
    while(1);
}
