.equ    BIOS_SRV_PANIC,  2

.equ BIOS_SRV_SYS, 8
.equ BIOS_SRV_BP, 9
.equ BIOS_SRV_HALT, 1
.equ BIOS_SRV_PANIC, 2
.equ BIOS_SRV_LDST, 3
.equ BIOS_SRV_WAIT, 4

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
    swi     #BIOS_SRV_PANIC  // PANIC code
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
    swi     #BIOS_SRV_HALT  // HALT code
    pop     {r0, lr}
    msr     spsr, r0
    mov     pc, lr

.global SYSCALL
SYSCALL:
//TODO: WARNING: CAUTION: we have to push lr and spsr onto the stack only if 
// we already are in svc mode.
    mrs     r7, spsr
    push    {r7, lr}
    swi     #BIOS_SRV_SYS
    pop     {r0, lr}
    msr     spsr, r0
    mov     pc, lr



.globl GETCPSR
GETCPSR:
    mrs r0, cpsr
    bx lr


.globl PUT32
PUT32:
    str r1,[r0]
    bx lr

.globl GET32
GET32:
    ldr r0,[r0]
    bx lr
