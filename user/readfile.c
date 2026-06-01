#include "libash.h"

void _start(void)
{
    /* open a file */
    int fd = sys_open("notes.txt");
    if (fd < 0) {
        println("Error: could not open notes.txt");
        sys_exit(1);
    }

    /* read it */
    char buf[256];
    int bytes = sys_read_fd(fd, buf, 255);
    if (bytes > 0) {
        buf[bytes] = 0;
        print("File contents: ");
        println(buf);
    }

    sys_close(fd);
    sys_exit(0);
    while(1);
}
