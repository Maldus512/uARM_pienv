.global _start

.section .init
_start:
    mov sp, #0x8000
    bl main

_hang:
    bl _hang


.end
