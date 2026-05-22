/*
 * This is our first userspace program.
 * It runs in ring 3 and communicates with the kernel via system calls.
 * It cannot call kernel functions directly.
 */

/* system call wrapper */
static void sys_write(const char* str, int len)
{
    __asm__ volatile (
        "mov $1, %%eax\n"
        "mov $0, %%ebx\n"
        "int $0x80\n"
        : : "c"(str), "d"(len) : "eax", "ebx"
    );
}

static void sys_exit(int code)
{
    __asm__ volatile (
        "int $0x80"
        : : "a"(0), "b"(code)
    );
}

void user_program(void)
{
    sys_write("Hello from ring 3!\n", 19);
    sys_write("I am a userspace program.\n", 26);
    sys_exit(0);

    /* should never reach here */
    while (1);
}
