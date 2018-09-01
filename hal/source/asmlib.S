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

.globl disable_irq
disable_irq:
    msr   daifset, #2
    ret

.globl STELR
STELR:
    msr     elr_el1, x0
    ret

.globl LDELR
LDELR:
    mrs     x0, elr_el1 
    ret


.globl PANIC
PANIC:
    wfe
    b PANIC