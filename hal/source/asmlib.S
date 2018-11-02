#include "bios_const.h"

.globl PUT32
PUT32:
  str w1,[x0]
  ret

.globl GET32
GET32:
    ldr w0,[x0]
    ret


.globl GETEL
GETEL:
    mrs x0,CurrentEL
    ret

.globl GETSAVEDSTATE
GETSAVEDSTATE:
    mrs x0,spsr_el1
    ret


.global WAIT
WAIT:
    wfi
    //mov     pc, lr
    ret

.global HALT
HALT:
    b HALT

.global SYSCALL
SYSCALL:
    svc #BIOS_SRV_SYS
    ret


.global GETARMCLKFRQ
GETARMCLKFRQ:
    mrs x0, cntfrq_el0
    ret

.global GETARMCOUNTER
GETARMCOUNTER:
    mrs x0, cntpct_el0
    ret


.globl enable_irq
enable_irq:
    msr   daifclr, #2
    ret

.globl enable_irq_el0
enable_irq_el0:
    mrs   x0, spsr_el1
    and   x0, x0, #0xFFFFFFFFFFFFFF7F //cler irq bit
    msr   spsr_el1, x0
    ret


.globl disable_irq
disable_irq:
    msr   daifset, #2
    ret

.globl STELR
STELR:
    mrs     x0, elr_el1 
    ret

.globl LDELR
LDELR:
    msr     elr_el1, x0
    ret


.globl PANIC
PANIC:
    wfe
    b PANIC

.globl STST
STST:
    adr     x20, .
//    mrs     x21, sp_el1
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
    str     w20, [x0, #4]!
    ret

.globl LDST
LDST:
    ldr     w0, [x0]
    ldr     w1, [x0, #4]!
    ldr     w2, [x0, #4]!
    ldr     w3, [x0, #4]!
    ldr     w4, [x0, #4]!
    ldr     w5, [x0, #4]!
    ldr     w6, [x0, #4]!
    ldr     w7, [x0, #4]!
    ldr     w8, [x0, #4]!
    ldr     w9, [x0, #4]!
    ldr     w10, [x0, #4]!
    ldr     w29, [x0, #4]! /* frame pointer */
    ldr     w11, [x0, #4]!
    ldr     x21, [x0, #4]!
    msr     sp_el1, x21    /* stack pointer (not accessible) */
    ldr     w30, [x0, #4]!  /* Link register */
    ldr     w20, [x0, #4]!
    msr     elr_el1, x20

.globl DOWFI
DOWFI:
    wfi
    //msr daifset,#2
    ret
