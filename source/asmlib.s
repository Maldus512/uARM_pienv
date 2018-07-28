.global GETSP
GETSP:
    // Return the stack pointer value
    str     sp, [sp]
    ldr     r0, [sp]
    // Return from the function
    mov     pc, lr

.global PANIC
PANIC:
//TODO: WARNING: CAUTION: we have to push lr and spsr onto the stack only if 
// we already are in svc mode.
    mrs     r0, spsr
    push    {r0, lr}
    swi     #0x2  // PANIC code
    pop     {r0, lr}
    msr     spsr, r0
    mov     pc, lr

.global WAIT
WAIT:
    //wfi
    mov r1, #0
    mcr p15, #0, r1, c7, c0, #4
    mov     pc, lr


.global HALT
HALT:
//TODO: WARNING: CAUTION: we have to push lr and spsr onto the stack only if 
// we already are in svc mode.
    mrs     r0, spsr
    push    {r0, lr}
    swi     #0x1  // HALT code
    pop     {r0, lr}
    msr     spsr, r0
    mov     pc, lr

.global SYSCALL
SYSCALL:
//TODO: WARNING: CAUTION: we have to push lr and spsr onto the stack only if 
// we already are in svc mode.
    mrs     r7, spsr
    push    {r7, lr}
    swi     #0x5 
    pop     {r0, lr}
    msr     spsr, r0
    mov     pc, lr



.globl GETCPSR
GETCPSR:
    mrs r0, cpsr
    bx lr
