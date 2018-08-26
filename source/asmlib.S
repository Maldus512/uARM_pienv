
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
    svc #0
    ret