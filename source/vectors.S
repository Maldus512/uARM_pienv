#include "system.h"
#include "bios_const.h"

.global hang
hang:
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
    vector keep_current_stack_syncronous_exception
    vector keep_current_stack_irq_handler
    stub 0x00000002
    stub 0x00000003

/* Current EL with SPX */
    vector keep_current_stack_syncronous_exception
    vector keep_current_stack_irq_handler
    stub 0x00000005
    stub 0x00000006

/* Lower EL using AARCH64 */
    vector syncronous_exception
    vector irq_handler
    stub 0x00000008
    stub 0x00000009

/* Lower EL using AARCH32 */
    vector syncronous_exception
    vector irq_handler
    stub 0x0000000e
    stub 0x0000000f

.balign 0x800
.global interrupt_vector_el2
interrupt_vector_el2:
/* Current EL with SP0 */
    stub 0x00000000
    stub 0x00000001
    stub 0x00000002
    stub 0x00000003

/* Current EL with SPX */
    stub 0x00000004
    stub 0x00000005
    stub 0x00000006
    stub 0x00000007

/* Lower EL using AARCH64 */
    stub 0x00000008
    stub 0x00000009
    vector fiq_handler_el2
    stub 0x0000000b

/* Lower EL using AARCH32 */
    stub 0x0000000c
    stub 0x0000000d
    stub 0x0000000e
    stub 0x0000000f


syncronous_exception:
    msr   daifset, #2
    mrs    x28, esr_el1                     // read the syndrome register
    lsr    x27, x28, #26      // exception class
    cmp    x27, #EC_SVC           // SVC in 64-bit state
    beq    swi_handler
unrecognized:
    mov    x0, x27
    and    x1, x28, #0x1FFFFFF
    b      abort_handler

keep_current_stack_syncronous_exception:
    msr   daifset, #2
    mrs    x28, esr_el1                     // read the syndrome register
    lsr    x27, x28, #26      // exception class
    cmp    x27, #EC_SVC           // SVC in 64-bit state
    beq    keep_current_stack_swi_handler
keep_current_stack_unrecognized:
    mov    x0, x27
    and    x1, x28, #0x1FFFFFF
    b      keep_current_stack_abort_handler


swi_handler:
    msr   daifset, #2
    msr   daifclr, #1

    mrs     x27, mpidr_el1
    and     x27, x27, #0x3
    cmp     x27, #0
    beq     set_swi_stack_core0
    cmp     x27, #1
    beq     set_swi_stack_core1
    cmp     x27, #2
    beq     set_swi_stack_core2
    cmp     x27, #3
    beq     set_swi_stack_core3

set_swi_stack_core0:
    ldr     x27, =__EL1_stack_core0
    b       set_chosen_swi_stack
set_swi_stack_core1:
    ldr     x27, =__EL1_stack_core1
    b       set_chosen_swi_stack
set_swi_stack_core2:
    ldr     x27, =__EL1_stack_core2
    b       set_chosen_swi_stack
set_swi_stack_core3:
    ldr     x27, =__EL1_stack_core3
set_chosen_swi_stack:
    mov     sp, x27
keep_current_stack_swi_handler:
    mrs     x28, ttbr0_el1
    mrs     x27, ttbr1_el1
    msr     ttbr0_el1, x27
    isb
    // save ttbr0
    str     x28, [sp, #-16]!
    // save link register and stack pointer
    stp     x29, x30, [sp, #-16]!
    // save x26 general purpose register
    str     x26, [sp, #-8]!

    // save state in oldarea
    mrs     x27, mpidr_el1
    and     x27, x27, #0x3
    mov   x28, #EXCEPTION_OLDAREA
    add   x28, x28, #SYNCHRONOUS_OFFSET
    mov     x26, #CORE_OFFSET
    madd   x28, x26, x27, x28
    ldr     x26, [sp], #8
    bl    STST_asm

    // call c handler.
    bl    c_swi_handler
    
    b hang

fiq_handler_el2:
    msr     daifset, #7

    /* save register values on the stack */
    stp   x29, x30, [sp, #-16]!
    bl    pushRegisters
    /* call c handler.  */
    bl    c_fiq_handler
    /* Restore registers */
    bl    popRegisters
    ldp   x29, x30, [sp], #16

    msr   daifclr, #4
    eret
   
/* Interrupts run at level EL1; they are enabled only at level EL0, so there are
    no nested interrupts */
irq_handler:
    msr   daifset, #3
    //msr   daifclr, #1

    mrs     x27, mpidr_el1
    and     x27, x27, #0x3
    cmp     x27, #0
    beq     set_irq_stack_core0
    cmp     x27, #1
    beq     set_irq_stack_core1
    cmp     x27, #2
    beq     set_irq_stack_core2
    cmp     x27, #3
    beq     set_irq_stack_core3

set_irq_stack_core0:
    ldr     x27, =__EL1_stack_core0
    b       set_chosen_stack
set_irq_stack_core1:
    ldr     x27, =__EL1_stack_core1
    b       set_chosen_stack
set_irq_stack_core2:
    ldr     x27, =__EL1_stack_core2
    b       set_chosen_stack
set_irq_stack_core3:
    ldr     x27, =__EL1_stack_core3
set_chosen_stack:
    mov     sp, x27
keep_current_stack_irq_handler:
    mrs     x28, ttbr0_el1
    mrs     x27, ttbr1_el1
    msr     ttbr0_el1, x27
    isb

    // save ttbr0
    str     x28, [sp, #-16]!
    // save link register and stack pointer
    stp     x29, x30, [sp, #-16]!
    // save x26 general purpose register
    str     x26, [sp, #-8]!
    
    mrs     x27, mpidr_el1
    and     x27, x27, #0x3
    mov   x28, #INTERRUPT_OLDAREA
    mov     x26, #CORE_OFFSET
    madd   x28, x26, x27, x28
    ldr     x26, [sp], #8
    bl    STST_asm

    // call c handler.
    bl    c_irq_handler
    /* The scheduler should manage returning */
    b hang


abort_handler:
    msr   daifset, #2
    msr   daifclr, #1

    mrs     x27, mpidr_el1
    and     x27, x27, #0x3
    cmp     x27, #0
    beq     set_abt_stack_core0
    cmp     x27, #1
    beq     set_abt_stack_core1
    cmp     x27, #2
    beq     set_abt_stack_core2
    cmp     x27, #3
    beq     set_abt_stack_core3

set_abt_stack_core0:
    ldr     x27, =__EL1_stack_core0
    b       set_chosen_stack
set_abt_stack_core1:
    ldr     x27, =__EL1_stack_core1
    b       set_chosen_stack
set_abt_stack_core2:
    ldr     x27, =__EL1_stack_core2
    b       set_chosen_stack
set_abt_stack_core3:
    ldr     x27, =__EL1_stack_core3
set_chosen_abt_stack:
    mov     sp, x27
keep_current_stack_abort_handler:
    mrs     x28, ttbr0_el1
    mrs     x27, ttbr1_el1
    msr     ttbr0_el1, x27
    isb

    // save ttbr0
    str     x28, [sp, #-16]!
    // save link and frame pointers
    stp     x29, x30, [sp, #-16]!
    // save x26 general purpose register
    str     x26, [sp, #-8]!
    
    mov   x28, #EXCEPTION_OLDAREA
    orr   x28, x28, #ABORT_OFFSET
    mov     x26, #CORE_OFFSET
    madd   x28, x26, x27, x28
    ldr     x26, [sp], #8
    bl    STST_asm

    // call c handler.
    bl    c_abort_handler
    b hang