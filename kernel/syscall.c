#include "../include/syscall.h"
#include "../include/process.h"
#include <stdint.h>

extern void terminal_writestring(const char* str);
extern void terminal_putchar(char c);
extern void kprintf(const char* fmt, ...);

/*
 * System call handler — called from the int 0x80 ISR.
 * regs->eax = syscall number
 * regs->ebx = arg1
 * regs->ecx = arg2
 * regs->edx = arg3
 */
void syscall_handler(registers_t* regs)
{
    switch (regs->eax) {

        case SYS_EXIT:
            kprintf("[PID %d exited with code %d]\n",
                    current_process->pid, regs->ebx);
            process_exit();
            break;

        case SYS_WRITE: {
            /*
             * ebx = fd (0 = stdout, ignored for now)
             * ecx = pointer to string
             * edx = length
             */
            const char* str = (const char*)regs->ecx;
            uint32_t    len = regs->edx;
            for (uint32_t i = 0; i < len; i++)
                terminal_putchar(str[i]);
            regs->eax = len; /* return bytes written */
            break;
        }

        case SYS_READ:
            /* placeholder for now */
            regs->eax = 0;
            break;

        default:
            kprintf("syscall: unknown call %d\n", regs->eax);
            regs->eax = (uint32_t)-1;
            break;
    }
}

void syscall_init(void)
{
    kprintf("Syscall: int 0x80 handler installed\n");
}
