#include "libuarmv2.h"
#include "uARMconst.h"
#include "bios_const.h"

.SECTION .init

.global _start

_start:
    // read cpu id, stop slave cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbz     x1, 2f
    // cpu id > 0, stop
1:  wfe
    b       1b
2:  // cpu id == 0

    /* Check exception level, start al EL0 */

    mrs     x0, CurrentEL
    and     x0, x0, #12

    /* Running at EL2? */
5:
    cmp     x0, #8
    bne     5f
    ldr     x1, =__EL2_stack
    mov     sp, x1
    ldr     x1, =__EL1_stack
    msr     sp_el1, x1
    ldr     x1, =__EL0_stack
    msr     sp_el0, x1
    mrs     x1, sp_el1

    /* Enable AArch64 in EL1 */
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs x0, hcr_el2

    mrs	x0, cnthctl_el2
	orr	x0, x0, #0x3						/* Enable EL1 access to timers */
	msr	cnthctl_el2, x0
	msr	cntvoff_el2, xzr
	
	mrs	x0, cntkctl_el1
	orr	x0, x0, #0x3						/* Enable EL0 access to timers */
    msr cntkctl_el1, x0

    /* change execution level to EL1 */
    //mrs     x0, SPSel
    mov     x2, #0x3c5
    msr     spsr_el2, x2
    adr     x2, 5f
    msr     elr_el2, x2
    eret

5:
    LDR    X1, = interrupt_vector
    MSR    VBAR_EL1, X1

    /*change execution level to EL0? */
    mov     x2, #0x3c0
    msr     spsr_el1, x2
    adr     x2, 3f
    msr     elr_el1, x2
    eret

    // jump to C code, should not return
3:  
    bl      _crt0
    // for failsafe, halt this core too
    b       hang

.global hang
hang:
    bl hexstring
    wfi
    b hang

.macro	vector handler
    .balign 0x80
    b	\handler
.endm

.macro stub code
    .balign 0x80
    mov x0, #\code
    b hang
.endm

.balign 0x800
.global interrupt_vector
interrupt_vector:
/* Current EL with SP0 */
    vector syncronousException
    vector irq_handler
    stub 0x00000002
    stub 0x00000003

/* Current EL with SPX */
    vector syncronousException
    vector irq_handler
    stub 0x00000005
    stub 0x00000006

/* Lower EL using AARCH64 */
    vector syncronousException
    vector irq_handler
    stub 0x00000008
    stub 0x00000009

/* Lower EL using AARCH32 */
    vector syncronousException
    vector irq_handler
    stub 0x0B
    stub 0x0C


syncronousException:
    mrs    x25, esr_el1                     // read the syndrome register
    lsr    x24, x25, #26      // exception class
    cmp    x24, #EC_SVC           // SVC in 64-bit state
    //bne    unrecognized
    //lsr    x24, x25, #5
    //cmp    x24, #BIOS_SRV_SYS
    beq    swi_handler
    //cmp    x24, #BIOS_INT_SYS
    //beq    intswi_handler
unrecognized:
    mov     x0, x24
    b      hang

swi_handler:
    stp   x0,  x1,  [sp, #-16]!
    stp   x2,  x3,  [sp, #-16]!
    stp   x4,  x5,  [sp, #-16]!
    stp   x6,  x7,  [sp, #-16]!
    stp   x8,  x9,  [sp, #-16]!
    stp   x10, x11, [sp, #-16]!
    stp   x12, x13, [sp, #-16]!
    stp   x14, x15, [sp, #-16]!
    stp   x16, x17, [sp, #-16]!
    stp   x18, x19, [sp, #-16]!
    str   x30, [sp, #-16]!
    // call c handler.
    bl    c_swi_handler
    mov   x20, x0 
    ldr   x30, [sp], #16
    ldp   x18, x19, [sp], #16
    ldp   x16, x17, [sp], #16
    ldp   x14, x15, [sp], #16
    ldp   x12, x13, [sp], #16
    ldp   x10, x11, [sp], #16
    ldp   x8,  x9,  [sp], #16
    ldp   x6,  x7,  [sp], #16
    ldp   x4,  x5,  [sp], #16
    ldp   x2,  x3,  [sp], #16
    ldp   x0,  x1,  [sp], #16
    mov   x0,  x20
    eret


/* Interrupts run at level EL1; they are enabled only at level EL0, so there are
    no nested interrupts */
irq_handler:
    msr   daifset, #2
    /* save register values on the stack */
    stp   x0,  x1,  [sp, #-16]!
    stp   x2,  x3,  [sp, #-16]!
    stp   x4,  x5,  [sp, #-16]!
    stp   x6,  x7,  [sp, #-16]!
    stp   x8,  x9,  [sp, #-16]!
    stp   x10, x11, [sp, #-16]!
    stp   x12, x13, [sp, #-16]!
    stp   x14, x15, [sp, #-16]!
    stp   x16, x17, [sp, #-16]!
    stp   x18, x19, [sp, #-16]!
    stp   x29, x30, [sp, #-16]!

    /* Save the old area uMPS way */
    mov     x0, #INT_OLDAREA
    mrs     x21, sp_el0  /* Stack pointer to be restored */
    mrs     x20, elr_el1 /* return link register, aka old program counter */
    str     w0, [x0]
    str     w1, [x0, #4]!
    str     w2, [x0, #4]!
    str     w3, [x0, #4]!
    str     w4, [x0, #4]!
    str     w5, [x0, #4]!
    str     w6, [x0, #4]!
    str     w7, [x0, #4]!
    str     w8, [x0, #4]!
    str     w9, [x0, #4]!
    str     w10, [x0, #4]!
    str     w29, [x0, #4]! /* frame pointer */
    str     w11, [x0, #4]! /* ip */
    str     w21, [x0, #4]! /* stack pointer (not accessible) */
    str     w30, [x0, #4]!  /* Link register */
    str     w20, [x0, #4]!  /* program counter */
    //TODO: missing status and stuff

    // call c handler.
    bl    c_irq_handler

    /* Ideally the scheduler should manage returning */
    ldp   x29, x30, [sp], #16
    ldp   x18, x19, [sp], #16
    ldp   x16, x17, [sp], #16
    ldp   x14, x15, [sp], #16
    ldp   x12, x13, [sp], #16
    ldp   x10, x11, [sp], #16
    ldp   x8,  x9,  [sp], #16
    ldp   x6,  x7,  [sp], #16
    ldp   x4,  x5,  [sp], #16
    ldp   x2,  x3,  [sp], #16
    ldp   x0,  x1,  [sp], #16
    //msr   daifclr, #2
    eret