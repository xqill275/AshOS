#ifndef LIBASH_H
#define LIBASH_H

/* ============================================================
   System call numbers
   ============================================================ */
#define SYS_EXIT  0
#define SYS_WRITE 1
#define SYS_READ  2
#define SYS_OPEN  3
#define SYS_CLOSE 4

/* ============================================================
   System call wrappers
   Use explicit register variables to avoid constraint conflicts
   ============================================================ */

static inline void sys_exit(int code)
{
    register int eax __asm__("eax") = SYS_EXIT;
    register int ebx __asm__("ebx") = code;
    __asm__ volatile ("int $0x80" : : "r"(eax), "r"(ebx));
    while(1);
}

static inline int sys_write(const char* str, int len)
{
    register int eax __asm__("eax") = SYS_WRITE;
    register int ebx __asm__("ebx") = 1; /* stdout */
    register const char* ecx __asm__("ecx") = str;
    register int edx __asm__("edx") = len;
    __asm__ volatile ("int $0x80"
        : "=a"(eax)
        : "r"(eax), "r"(ebx), "r"(ecx), "r"(edx));
    return eax;
}

static inline int sys_open(const char* filename)
{
    register int eax __asm__("eax") = SYS_OPEN;
    register const char* ebx __asm__("ebx") = filename;
    __asm__ volatile ("int $0x80"
        : "=a"(eax)
        : "r"(eax), "r"(ebx));
    return eax;
}

static inline int sys_read_fd(int fd, char* buf, int size)
{
    register int eax __asm__("eax") = SYS_READ;
    register int ebx __asm__("ebx") = fd;
    register char* ecx __asm__("ecx") = buf;
    register int edx __asm__("edx") = size;
    __asm__ volatile ("int $0x80"
        : "=a"(eax)
        : "r"(eax), "r"(ebx), "r"(ecx), "r"(edx));
    return eax;
}

static inline void sys_close(int fd)
{
    register int eax __asm__("eax") = SYS_CLOSE;
    register int ebx __asm__("ebx") = fd;
    __asm__ volatile ("int $0x80" : : "r"(eax), "r"(ebx));
}

/* ============================================================
   Higher level helpers
   ============================================================ */

static inline int strlen(const char* str)
{
    int len = 0;
    while (str[len]) len++;
    return len;
}

static inline void print(const char* str)
{
    sys_write(str, strlen(str));
}

static inline void println(const char* str)
{
    print(str);
    sys_write("\n", 1);
}

#endif
