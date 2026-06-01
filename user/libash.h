#ifndef LIBASH_H
#define LIBASH_H

/* ============================================================
   System call numbers
   ============================================================ */
#define SYS_EXIT  0
#define SYS_WRITE 1
#define SYS_READ  2

/* ============================================================
   System call wrappers
   ============================================================ */

static inline void sys_exit(int code)
{
    __asm__ volatile (
        "mov $0, %%eax\n"
        "int $0x80\n"
        : : "b"(code) : "eax"
    );
}

static inline int sys_write(const char* str, int len)
{
    int ret;
    __asm__ volatile (
        "mov $1, %%eax\n"
        "mov $0, %%ebx\n"
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=r"(ret) : "c"(str), "d"(len) : "eax", "ebx"
    );
    return ret;
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
