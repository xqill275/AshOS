#include "../include/syscall.h"
#include "../include/process.h"
#include "../include/fs.h"
#include "../include/heap.h"
#include <stdint.h>

extern void terminal_putchar(char c);
extern void kprintf(const char* fmt, ...);

void syscall_handler(registers_t* regs)
{
    switch (regs->eax) {

        case SYS_EXIT:
            kprintf("[PID %d exited with code %d]\n",
                    current_process->pid, regs->ebx);
            process_exit();
            break;

        case SYS_WRITE: {
            uint32_t fd  = regs->ebx;
            const char* str = (const char*)regs->ecx;
            uint32_t len = regs->edx;
            if (fd == 1 || fd == 2) {
                /* stdout / stderr - write to terminal */
                for (uint32_t i = 0; i < len; i++)
                    terminal_putchar(str[i]);
                regs->eax = len;
            } else if (fd < MAX_FD && current_process->fds[fd].used) {
                /* write to file */
                fs_write(current_process->fds[fd].name,
                         (const uint8_t*)str, len);
                regs->eax = len;
            } else {
                regs->eax = (uint32_t)-1;
            }
            break;
        }

        case SYS_READ: {
            uint32_t fd   = regs->ebx;
            uint8_t* buf  = (uint8_t*)regs->ecx;
            uint32_t size = regs->edx;
            if (fd < MAX_FD && current_process->fds[fd].used) {
                int bytes = fs_read(current_process->fds[fd].name, buf, size);
                regs->eax = bytes;
            } else {
                regs->eax = (uint32_t)-1;
            }
            break;
        }

        case SYS_OPEN: {
            const char* filename = (const char*)regs->ebx;

            /* copy filename into a safe kernel buffer first */
            char safe_name[64];
            int n = 0;
            while (filename[n] && n < 63) {
                safe_name[n] = filename[n];
                n++;
            }
            safe_name[n] = 0;
            /* find a free fd slot starting from 3 */
            int fd = -1;
            for (int i = 3; i < MAX_FD; i++) {
                if (!current_process->fds[i].used) {
                    fd = i;
                    break;
                }
            }

            if (fd < 0) {
                regs->eax = (uint32_t)-1;
                break;
            }

            /* check file exists */
            uint8_t* tmp = (uint8_t*)kmalloc(512);
            int bytes = fs_read(safe_name, tmp, 512);
            kfree(tmp);

            if (bytes < 0) {
                regs->eax = (uint32_t)-1;
                break;
            }

            /* fill in the fd using safe_name */
            current_process->fds[fd].used     = 1;
            current_process->fds[fd].position = 0;
            current_process->fds[fd].size     = bytes;
            for (int i = 0; i <= n; i++)
                current_process->fds[fd].name[i] = safe_name[i];

            regs->eax = fd;
            break;
        }

        case SYS_CLOSE: {
            uint32_t fd = regs->ebx;
            if (fd >= 3 && fd < MAX_FD && current_process->fds[fd].used) {
                current_process->fds[fd].used = 0;
                regs->eax = 0;
            } else {
                regs->eax = (uint32_t)-1;
            }
            break;
        }

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
