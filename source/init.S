#include "system.h"
#include "bios_const.h"

.SECTION .init

.global _start
_start:
    /* Assuming to start at EL2
       Setup various cores and exception levels stack */

    ldr     x1, =CoresReady
    mov     x0, #1
    str     w0, [x1]

multicore_start:

    mov x0, #1
    msr ttbr0_el1, x0
    mov x0, #2
    msr ttbr1_el1, x0

    mrs	x0, cnthctl_el2
    orr	x0, x0, #0x3		/* Enable EL1 access to timers */
    msr	cnthctl_el2, x0
    msr	cntvoff_el2, xzr

    mrs	x0, cntkctl_el1
    orr	x0, x0, #0x3		/* Enable EL0 access to timers */
    msr cntkctl_el1, x0


    ldr     x3, =__EL1_stack_core0
    mrs     x6, mpidr_el1           // Read core id

    ands    x6, x6, #0x3
    beq     set_stacks

    ldr     x3, =__EL1_stack_core1
    cmp     x6, #1	
    beq     set_stacks

    ldr     x3, =__EL1_stack_core2
    cmp     x6, #2	
    beq     set_stacks

    ldr     x3, =__EL1_stack_core3
    cmp     x6, #3	
    beq     set_stacks

set_stacks:
    msr     sp_el1, x3

    /* Enable AArch64 in EL1 */
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs     x0, hcr_el2

    /* change execution level to EL1 */
    mov     x2, #0x305
    msr     spsr_el2, x2
    adr     x2, start_kernel
    msr     elr_el2, x2
    eret

start_kernel:
    ldr    x1, =interrupt_vector
    msr    vbar_el1, x1

    mrs     x6, mpidr_el1
    and     x6, x6, #0x3
    /* The "main" kernel runs on core0 */
    cbz     x6, start_kernel_core0

    ldr     x1, =CoresReady
    ldr     w0, [x1]
    add     w0, w0, #1
    str     w0, [x1]
    b       start_idle_core_spin

start_kernel_core0:
    mov     x1, #spin_cpu1
    ldr     x2, =multicore_start
    str     x2, [x1]
    sev
    ldr     x3, =CoresReady
.wait_ack_core1:
    ldr     w1, [x3]
    cmp     w1, #2
    bne     .wait_ack_core1

    mov     x1, #spin_cpu2
    ldr     x2, =multicore_start
    str     x2, [x1]
    sev
    ldr     x3, =CoresReady
.wait_ack_core2:
    ldr     w1, [x3]
    cmp     w1, #3
    bne     .wait_ack_core2

    mov     x1, #spin_cpu3
    ldr     x2, =multicore_start
    str     x2, [x1]
    sev
    ldr     x3, =CoresReady
.wait_ack_core3:
    ldr     w1, [x3]
    cmp     w1, #4
    bne     .wait_ack_core3

    // jump to C code, should not return
    bl      _crt0
    // for failsafe, halt this core too
    b       hang

.global hang
hang:
    bl hexstring
hang2:
    wfi
    b hang2

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
    vector keep_current_stack_irq_handler
    vector fiq_handler
    stub 0x00000003

/* Current EL with SPX */
    vector syncronousException
    vector keep_current_stack_irq_handler
    vector fiq_handler
    stub 0x00000006

/* Lower EL using AARCH64 */
    vector syncronousException
    vector irq_handler
    vector fiq_handler
    stub 0x00000009

/* Lower EL using AARCH32 */
    vector syncronousException
    vector irq_handler
    vector fiq_handler
    stub 0x0C


syncronousException:
    msr   daifset, #2
    mrs    x28, esr_el1                     // read the syndrome register
    lsr    x27, x28, #26      // exception class
    cmp    x27, #EC_SVC           // SVC in 64-bit state
    beq    swi_handler
unrecognized:
    mov    x0, x27
    and    x1, x28, #0x1FFFFFF
    b      abort_handler

swi_handler:
    msr   daifset, #2
    msr   daifclr, #1

    // if coming from level other than EL0 keep the current stack
    mrs     x27, spsr_el1
    and     x27, x27, #7
    cmp     x27, #0
    bne     keep_current_stack_swi

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
keep_current_stack_swi:
    mrs     x28, ttbr0_el1
    mrs     x27, ttbr1_el1
    msr     ttbr0_el1, x27
    // save ttbr0
    str     x28, [sp, #-16]!
    // save link register and stack pointer
    stp     x29, x30, [sp, #-16]!

    // save state in oldarea
    mrs     x27, mpidr_el1
    and     x27, x27, #0x3
    mov   x28, #EXCEPTION_OLDAREA
    add   x28, x28, #SYNCHRONOUS_OFFSET
    mov     x26, #CORE_OFFSET
    madd   x28, x26, x27, x28
    //madd   x28, #CORE_OFFSET, x27, x28
    bl    STST_asm

    // call c handler.
    bl    c_swi_handler
    
    b hang


fiq_handler:
    msr   daifset, #3

    mrs     x28, ttbr0_el1
    mrs     x27, ttbr1_el1
    msr     ttbr0_el1, x27
    /* save register values on the stack */
    stp   x29, x30, [sp, #-16]!
    bl    pushRegisters

    // call c handler.
    bl    c_fiq_handler

    /* Ideally the scheduler should manage returning */
    bl    popRegisters
    ldp   x29, x30, [sp], #16

    msr     ttbr0_el1, x28

    //msr   daifclr, #1
    eret
    
/* Interrupts run at level EL1; they are enabled only at level EL0, so there are
    no nested interrupts */
irq_handler:
    msr   daifset, #2
    msr   daifclr, #1

    // if coming from level other than EL0 keep the current stack
    mrs     x27, spsr_el1
    and     x27, x27, #7
    cmp     x27, #0
    bne     keep_current_stack_irq_handler
    
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

    // save ttbr0
    str     x28, [sp, #-16]!
    // save link register and stack pointer
    stp     x29, x30, [sp, #-16]!
    
    mrs     x27, mpidr_el1
    and     x27, x27, #0x3
    mov   x28, #INTERRUPT_OLDAREA
    mov     x26, #CORE_OFFSET
    madd   x28, x26, x27, x28
    bl    STST_asm

    // call c handler.
    bl    c_irq_handler
    /* The scheduler should manage returning */
    b hang


abort_handler:
    msr   daifset, #2
    msr   daifclr, #1

    // if coming from level other than EL0 keep the current stack
    mrs     x27, spsr_el1
    and     x27, x27, #7
    cmp     x27, #0
    bne     keep_current_stack_abort

    mrs     x27, ttbr0_el1
    
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
keep_current_stack_abort:
    mrs     x28, ttbr0_el1
    mrs     x27, ttbr1_el1
    msr     ttbr0_el1, x27
    // save link and frame pointers
    stp     x29, x30, [sp, #-16]!
    // save ttbr0
    str     x28, [sp, #-16]!
    
    mov   x28, #EXCEPTION_OLDAREA
    orr   x28, x28, #ABORT_OFFSET
    mov     x26, #CORE_OFFSET
    madd   x28, x26, x27, x28
    bl    STST_asm

    // call c handler.
    bl    c_abort_handler
    b hang


.balign 4
start_idle_core_spin:
    mrs     x6, mpidr_el1
    and     x6, x6, #0x3
    mov     x5, #spin_cpu0
    mov     x1, #0
    str     x1, [x5, x6, lsl #3]
idle_core_spin:
    wfe

    ldr     x4, [x5, x6, lsl #3]
    cbz     x4, idle_core_spin
    mov     x0, #0
    str     x0, [x5, x6, lsl #3]
    mov     x1, #0
    mov     x2, #0
    mov     x3, #0
    blr     x4
    b       start_idle_core_spin


.section ".data.init", "aw"
.balign 4

.globl CoresReady;
CoresReady : .4byte 0;
