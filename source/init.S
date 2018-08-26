/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

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

    /* Enable AArch64 in EL1 */
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs x0, hcr_el2
    /* change execution level to EL1 */
    mov     x2, #0x3c4
    msr     spsr_el2, x2
    adr     x2, 5f
    msr     elr_el2, x2
    eret

5:
    LDR    X1, = interrupt_vector
    MSR    VBAR_EL1, X1
    /*ldr     x1, =__EL1_stack
    mov     sp, x1*/

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
    vector swi_handler
    stub 0x01
    stub 0x02
    stub 0x03

/* Current EL with SPX */
    vector swi_handler
    stub 0x04
    stub 0x05
    stub 0x06

/* Lower EL using AARCH64 */
    vector swi_handler
    stub 0x07
    stub 0x08
    stub 0x09

/* Lower EL using AARCH32 */
    vector swi_handler
    stub 0x0A
    stub 0x0B
    stub 0x0C

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
    eret