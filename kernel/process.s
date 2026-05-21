.global context_switch

/*
 * context_switch(uint32_t* old_esp, uint32_t new_esp)
 * Saves current register state onto the current stack,
 * switches to the new stack, then restores register state.
 */
context_switch:
    /* get arguments */
    mov 4(%esp), %eax   /* old_esp pointer */
    mov 8(%esp), %ecx   /* new_esp value */

    /* save current registers onto current stack */
    pusha

    /* save current stack pointer into old process esp */
    mov %esp, (%eax)

    /* switch to new stack */
    mov %ecx, %esp

    /* restore new process registers */
    popa

    ret
