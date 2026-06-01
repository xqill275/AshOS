#include "../include/process.h"
#include "../include/heap.h"
#include "../include/vmm.h"
#include <stdint.h>
#include <stddef.h>

extern void kprintf(const char* fmt, ...);

/* current running process */
process_t* current_process = 0;

/* head of the process list */
process_t* process_list = 0;

/* pid counter */
static uint32_t next_pid = 0;

/* ============================================================
   Context switch - defined in process.s
   ============================================================ */
extern void context_switch(uint32_t* old_esp, uint32_t new_esp);

/* ============================================================
   Process management
   ============================================================ */

void process_init(void)
{
    /*
     * Create a process for the kernel itself so the scheduler
     * has something to switch back to.
     */
    process_t* kernel_proc = (process_t*)kmalloc(sizeof(process_t));
    kernel_proc->pid       = next_pid++;
    kernel_proc->state     = PROCESS_RUNNING;
    kernel_proc->next      = 0;
    kernel_proc->stack     = 0; /* kernel already has a stack */
    kernel_proc->page_dir  = 0; /* kernel uses current page dir */
    kernel_proc->esp       = 0;
    for (int i = 0; i < MAX_FD; i++) {
        kernel_proc->fds[i].used = 0;
    }
    kernel_proc->fds[0].used = 1;
    kernel_proc->fds[1].used = 1;
    kernel_proc->fds[2].used= 1;
    process_list    = kernel_proc;
    current_process = kernel_proc;

    kprintf("Process: kernel process created (PID 0)\n");
}

process_t* process_create(void (*entry)(void))
{
    /* allocate process struct */
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if (!proc) return 0;

    /* allocate kernel stack */
    proc->stack = (uint8_t*)kmalloc(KERNEL_STACK_SIZE);
    if (!proc->stack) {
        kfree(proc);
        return 0;
    }

    proc->pid      = next_pid++;
    proc->state    = PROCESS_READY;
    proc->page_dir = 0;
    proc->next     = 0;

    /* initialise file descriptors */
    for (int i = 0; i < MAX_FD; i++)
        proc->fds[i].used = 0;
    proc->fds[0].used = 1; /* stdin */
    proc->fds[1].used = 1; /* stdout */
    proc->fds[2].used = 1; /* stderr */

    /*
     * Set up the initial stack so context_switch can restore it.
     * We push the entry point and fake a return address.
     * Stack grows downward so we start at the top.
     */
    uint32_t* stack_top = (uint32_t*)(proc->stack + KERNEL_STACK_SIZE);

    /* push entry point as if it was called */
    *(--stack_top) = (uint32_t)entry;  /* eip */

    /* push fake values for all registers pusha would save */
    *(--stack_top) = 0; /* eax */
    *(--stack_top) = 0; /* ecx */
    *(--stack_top) = 0; /* edx */
    *(--stack_top) = 0; /* ebx */
    *(--stack_top) = 0; /* esp (ignored by popa) */
    *(--stack_top) = 0; /* ebp */
    *(--stack_top) = 0; /* esi */
    *(--stack_top) = 0; /* edi */

    proc->esp = (uint32_t)stack_top;

    /* add to end of process list */
    process_t* curr = process_list;
    while (curr->next)
        curr = curr->next;
    curr->next = proc;

    kprintf("Process: created PID %d\n", proc->pid);
    return proc;
}

void process_exit(void)
{
    current_process->state = PROCESS_DEAD;
    /* scheduler will skip dead processes */
    while (1) __asm__ volatile ("hlt");
}

/* ============================================================
   Scheduler
   ============================================================ */

void scheduler_tick(void)
{
    if (!current_process) return;

    /* find next ready process */
    process_t* next = current_process->next;

    /* wrap around to start of list */
    if (!next) next = process_list;

    /* skip dead processes */
    while (next->state == PROCESS_DEAD && next != current_process)
        next = next->next ? next->next : process_list;

    if (next == current_process) return; /* only one process */

    process_t* prev = current_process;
    current_process = next;
    current_process->state = PROCESS_RUNNING;

    if (prev->state != PROCESS_DEAD)
        prev->state = PROCESS_READY;

    /* switch context */
    context_switch(&prev->esp, current_process->esp);
}
