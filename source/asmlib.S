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



.global PANIC
PANIC:
    b PANIC

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