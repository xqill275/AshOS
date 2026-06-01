#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define KERNEL_STACK_SIZE 4096
#define MAX_PROCESSES     16
#define MAX_FD 16

typedef struct {
    uint8_t  used;
    char     name[64];
    uint32_t position;
    uint32_t size;
} file_descriptor_t;

typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_DEAD
} process_state_t;

typedef struct process {
    uint32_t        pid;
    uint32_t        esp;        /* saved stack pointer */
    uint32_t*       page_dir;  /* page directory */
    uint8_t*        stack;     /* kernel stack */
    process_state_t state;
    file_descriptor_t fds[MAX_FD];
    struct process* next;      /* next process in list */
} process_t;

extern process_t* current_process;  /* no static keyword */
extern process_t* process_list;
void     process_init(void);
process_t* process_create(void (*entry)(void));
void     process_exit(void);
void     scheduler_tick(void);

#endif
