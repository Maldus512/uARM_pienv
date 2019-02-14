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
    mrs x0, CurrentEl
    ands x0, x0, #0xC
    cmp x0, #0x8
    beq already_el2
    
    mov     x0, #0x309
    msr     spsr_el2, x0
    adr     x0, already_el2
    msr     elr_el2, x0
    eret

already_el2:
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
    mov     sp, x3

    /* Enable AArch64 in EL1 */
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs     x0, hcr_el2

    ldr    x1, =interrupt_vector
    msr    vbar_el1, x1
    msr    vbar_el2, x1

    /* change execution level to EL1 */
    mov     x2, #0x305
    msr     spsr_el2, x2
    adr     x2, start_kernel
    msr     elr_el2, x2
    eret

start_kernel:
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
