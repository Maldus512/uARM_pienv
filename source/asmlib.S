
.globl PUT32
PUT32:
  str w1,[x0]
  ret

.globl GET32
GET32:
    ldr w0,[x0]
    ret

