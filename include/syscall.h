#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "../include/registers.h"
/* system call numbers */
#define SYS_EXIT  0
#define SYS_WRITE 1
#define SYS_READ  2

void syscall_init(void);
void syscall_handler(registers_t* regs);

#endif
