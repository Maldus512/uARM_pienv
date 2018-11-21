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

.globl GETSAVEDEL
GETSAVEDEL:
    mrs x0, spsr_el1
    and x0, x0, #7
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
    svc #0//BIOS_SRV_SYS
    ret

.global INTERNALSYSCALL
INTERNALSYSCALL:
    //svc #BIOS_INT_SYS
    svc #0x00ff
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
    and   x0, x0, #0xFFFFFFFFFFFFFF7F //clear irq bit
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



.globl DOWFI
DOWFI:
    wfi
    //msr daifset,#2
    ret

.globl GETSP_EL0
GETSP_EL0:
    mrs    x0, sp_el0
    ret


.globl STST
STST:
    mov     x21, sp
    adr     x20, .
    //mrs     x21, sp_el0
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
    //TODO: missing status and stuff
    ret

.globl LDST_EL0
LDST_EL0:
    mov     x22, x0
    ldr     w0, [x22]
    ldr     w1, [x22, #4]!
    ldr     w2, [x22, #4]!
    ldr     w3, [x22, #4]!
    ldr     w4, [x22, #4]!
    ldr     w5, [x22, #4]!
    ldr     w6, [x22, #4]!
    ldr     w7, [x22, #4]!
    ldr     w8, [x22, #4]!
    ldr     w9, [x22, #4]!
    ldr     w10, [x22, #4]!
    ldr     w29, [x22, #4]! /* frame pointer */
    ldr     w11, [x22, #4]!
    ldr     w21, [x22, #4]! /* stack pointer (not accessible) */
    ldr     w30, [x22, #4]!  /* Link register */
    ldr     w20, [x22, #4]! /* program counter (to be loaded in the exception link register) */
    msr     elr_el1, x20
    msr     sp_el0, x21
    /*change execution level to EL0? */
    mov     x2, #0x340
    /*
    mov     x2, #0x3c0
    0x3c0 has the interrupt bit set, it has to be clear to enable ints*/
    msr     spsr_el1, x2
    eret



.globl ENABLEMMU
ENABLEMMU:
    // finally, toggle some bits in system control register to enable page translation
    isb
    mrs x0, sctlr_el1
    orr x0, x0, #1
    msr sctlr_el1, x0
    isb //permission fault, level 2 (esr_el1 = 0x8600000e)
    ret