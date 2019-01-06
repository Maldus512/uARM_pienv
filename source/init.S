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

    mrs	x0, cnthctl_el2
    orr	x0, x0, #0x3		/* Enable EL1 access to timers */
    msr	cnthctl_el2, x0
    msr	cntvoff_el2, xzr

    mrs	x0, cntkctl_el1
    orr	x0, x0, #0x3		/* Enable EL0 access to timers */
    msr cntkctl_el1, x0


    ldr     x2, =__EL2_stack
    ldr     x3, =__EL1_stack_core0
    ldr     x4, =__EL0_stack
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

//TODO: check if I can be at some other level than EL2 here
set_stacks:
    mov     sp, x2
    msr     sp_el1, x3
    msr     sp_el0, x4

    /* Those instructions apparently have no effect */
	mrs	x0, midr_el1
	mrs	x1, mpidr_el1
	msr	vpidr_el2, x0
    msr vmpidr_el2, x1

    /*  Disable coprocessor traps for all Cores */
	mov	x0, #0x33ff
	msr	cptr_el2, x0						// Disable coprocessor traps to EL2
	msr	hstr_el2, xzr						// Disable coprocessor traps to EL2
	mov	x0, #3 << 20
    msr cpacr_el1, x0 // Enable FP/SIMD at EL1

    /* Enable AArch64 in EL1 */
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs     x0, hcr_el2

    /* change execution level to EL1 */
    mov     x2, #0x3C5
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

    //TODO: enable when everything works
    //mrs    x1, sctlr_el1
    /* Enable instruction cache */
    //orr    w1, w1, #0x1000
    //msr    sctlr_el1, x1

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
    msr   daifset, #2
    mrs    x25, esr_el1                     // read the syndrome register
    lsr    x24, x25, #26      // exception class
    cmp    x24, #EC_SVC           // SVC in 64-bit state
    beq    swi_handler
unrecognized:
    mov    x0, x24
    and    x1, x25, #0x1FFFFFF
    b      c_abort_handler

swi_handler:
    msr   daifset, #2

    mrs     x10, mpidr_el1
    and     x10, x10, #0x3
    cmp     x10, #0
    beq     set_swi_stack_core0
    cmp     x10, #1
    beq     set_swi_stack_core1
    cmp     x10, #2
    beq     set_swi_stack_core2
    cmp     x10, #3
    beq     set_swi_stack_core3

set_swi_stack_core0:
    ldr     x10, =__EL1_stack_core0
    b       set_chosen_swi_stack
set_swi_stack_core1:
    ldr     x10, =__EL1_stack_core1
    b       set_chosen_swi_stack
set_swi_stack_core2:
    ldr     x10, =__EL1_stack_core2
    b       set_chosen_swi_stack
set_swi_stack_core3:
    ldr     x10, =__EL1_stack_core3
set_chosen_swi_stack:
    mov     sp, x10
    // save state in oldarea
    stp   x27, x28, [sp, #-16]
    mov   x28, #INTERRUPT_OLDAREA
    orr   x28, x28, #SYNCHRONOUS_OFFSET
    stp   x0, x1, [x28]
    stp   x2, x3, [x28, #16]!
    stp   x4, x5, [x28, #16]!
    stp   x6, x7, [x28, #16]!
    stp   x8, x9, [x28, #16]!
    stp   x10, x11, [x28, #16]!
    stp   x12, x13, [x28, #16]!
    stp   x14, x15, [x28, #16]!
    stp   x16, x17, [x28, #16]!
    stp   x18, x19, [x28, #16]!
    stp   x20, x21, [x28, #16]!
    stp   x22, x23, [x28, #16]!
    stp   x24, x25, [x28, #16]!
    stp   x26, x27, [x28, #16]!

    ldp   x26, x27, [sp, #-16]
    stp   x27, x29, [x28, #16]! // Save x28 properly

    mrs   x27, spsr_el1
    and   x27, x27, #7
    cmp   x27, #0
    bne   savesp_swi_el1
    mrs   x27, sp_el0
    b     finish_swi
savesp_swi_el1:
    mov   x27, sp
finish_swi:
    stp   x30, x27, [x28, #16]!
    
    mrs   x27, elr_el1
    str   x27, [x28, #16]! // Note: the 16 immediate value refers to the previously saved pair

    mrs   x27, ttbr0_el1
    str   x27, [x28, #8]!

    mrs   x27, spsr_el1
    str   x27, [x28, #8]!

    ldp   x27, x28, [sp, #-16]

    str   x1,  [sp, #-16]!
    stp   x2,  x3,  [sp, #-16]!
    stp   x4,  x5,  [sp, #-16]!
    stp   x6,  x7,  [sp, #-16]!
    stp   x8,  x9,  [sp, #-16]!
    stp   x10, x11, [sp, #-16]!
    stp   x12, x13, [sp, #-16]!
    stp   x14, x15, [sp, #-16]!
    stp   x16, x17, [sp, #-16]!
    stp   x18, x19, [sp, #-16]!
    stp   x20, x21, [sp, #-16]!
    stp   x22, x23, [sp, #-16]!
    stp   x24, x25, [sp, #-16]!
    stp   x26, x27, [sp, #-16]!
    stp   x28, x29, [sp, #-16]!
    str   x30, [sp, #-16]!
    // call c handler.
    bl    c_swi_handler
    ldr   x30, [sp], #16
    ldp   x28, x29, [sp], #16
    ldp   x26, x27, [sp], #16
    ldp   x24, x25, [sp], #16
    ldp   x22, x23, [sp], #16
    ldp   x20, x21, [sp], #16
    ldp   x18, x19, [sp], #16
    ldp   x16, x17, [sp], #16
    ldp   x14, x15, [sp], #16
    ldp   x12, x13, [sp], #16
    ldp   x10, x11, [sp], #16
    ldp   x8,  x9,  [sp], #16
    ldp   x6,  x7,  [sp], #16
    ldp   x4,  x5,  [sp], #16
    ldp   x2,  x3,  [sp], #16
    ldr   x1,  [sp], #16
    eret


/* Interrupts run at level EL1; they are enabled only at level EL0, so there are
    no nested interrupts */
irq_handler:
    msr   daifset, #2
    
    mrs     x10, mpidr_el1
    and     x10, x10, #0x3
    cmp     x10, #0
    beq     set_irq_stack_core0
    cmp     x10, #1
    beq     set_irq_stack_core1
    cmp     x10, #2
    beq     set_irq_stack_core2
    cmp     x10, #3
    beq     set_irq_stack_core3

set_irq_stack_core0:
    ldr     x10, =__EL1_stack_core0
    b       set_chosen_stack
set_irq_stack_core1:
    ldr     x10, =__EL1_stack_core1
    b       set_chosen_stack
set_irq_stack_core2:
    ldr     x10, =__EL1_stack_core2
    b       set_chosen_stack
set_irq_stack_core3:
    ldr     x10, =__EL1_stack_core3
set_chosen_stack:
    mov     sp, x10
    // save state in oldarea
    stp   x27, x28, [sp, #-16]
    mov   x28, #INTERRUPT_OLDAREA
    stp   x0, x1, [x28]
    stp   x2, x3, [x28, #16]!
    stp   x4, x5, [x28, #16]!
    stp   x6, x7, [x28, #16]!
    stp   x8, x9, [x28, #16]!
    stp   x10, x11, [x28, #16]!
    stp   x12, x13, [x28, #16]!
    stp   x14, x15, [x28, #16]!
    stp   x16, x17, [x28, #16]!
    stp   x18, x19, [x28, #16]!
    stp   x20, x21, [x28, #16]!
    stp   x22, x23, [x28, #16]!
    stp   x24, x25, [x28, #16]!
    stp   x26, x27, [x28, #16]!

    ldp   x26, x27, [sp, #-16]
    stp   x27, x29, [x28, #16]! // Save x28 properly

    mrs   x27, spsr_el1
    and   x27, x27, #7
    cmp   x27, #0
    bne   savesp_el1
    mrs   x27, sp_el0
    b     finish
savesp_el1:
    mov   x27, sp
finish:
    stp   x30, x27, [x28, #16]!
    
    mrs   x27, elr_el1
    str   x27, [x28, #16]! // Note: the 16 immediate value refers to the previously saved pair

    mrs   x27, ttbr0_el1
    str   x27, [x28, #8]!

    mrs   x27, spsr_el1
    str   x27, [x28, #8]!

    ldp   x27, x28, [sp, #-16]

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
    stp   x20, x21, [sp, #-16]!
    stp   x22, x23, [sp, #-16]!
    stp   x24, x25, [sp, #-16]!
    stp   x26, x27, [sp, #-16]!
    stp   x28, x29, [sp, #-16]!
    str   x30, [sp, #-16]!

    // call c handler.
    bl    c_irq_handler

    /* Ideally the scheduler should manage returning */
    ldr   x30, [sp], #16
    ldp   x28, x29, [sp], #16
    ldp   x26, x27, [sp], #16
    ldp   x24, x25, [sp], #16
    ldp   x22, x23, [sp], #16
    ldp   x20, x21, [sp], #16
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

.globl CoreExecute
CoreExecute:
	ands x0, x0, #255
	beq CoreExecuteFail
	ldr x3, =CoresReady
	ldr w2, [x3]							// Fetch cores ready count
	cmp w0, w2
	bcs	CoreExecuteFail
	mov x6, #0
	mov w6, w0
	mov x5, #spin_cpu0						// Load address of spins
	str x1, [x5, x6, lsl #3]				// Save caller address
	dsb sy
	sev
	mov x0, #1
	ret
CoreExecuteFail:
	mov x0, #0
        ret

.section ".data.init", "aw"
.balign 4

.globl CoresReady;
CoresReady : .4byte 0;
