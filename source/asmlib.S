#include "bios_const.h"

.globl GETEL
GETEL:
    mrs x0,CurrentEL
    ret

.globl ISMMUACTIVE
ISMMUACTIVE:
    mrs x0, sctlr_el1
    and x0, x0, #1
    ret

.globl GETSAVEDEL
GETSAVEDEL:
    mov x0, #0
    mrs x0, spsr_el1
    and x0, x0, #0x7
    ret

.globl GETSP
GETSP:
    mov x0, sp
    ret

.globl SETSP
SETSP:
    //mov sp, x0
    ret

.globl GETTTBR0
GETTTBR0:
    mrs x0, ttbr0_el1
    ret

.globl GETTTBR1
GETTTBR1:
    mrs x0, ttbr1_el1
    ret

.global wait_lock
.global WAIT
WAIT:
    dsb ish
    ldr  x0, =wait_lock
    mrs  x1, mpidr_el1
    and  x1, x1, #0x3
    mov  x2, #16
    madd x0, x1, x2, x0
    mov  x1, 0
    str  x1, [x0]
waitloop:
    wfi
    ldr  x1, [x0]
    cmp  x1, #1 
    b.ne  waitloop

    ret

.global HALT
HALT:
    wfi
    b HALT

.global SYSCALL
SYSCALL:
    svc #0
    ret

.global GETARMCLKFRQ
GETARMCLKFRQ:
    mrs x0, cntfrq_el0
    ret

.globl PANIC
PANIC:
    wfe
    b PANIC

.globl getCORE
getCORE:
    mrs     x0, mpidr_el1
    and     x0, x0, #0x3
    ret

.globl STST
STST:
    /* General purpose registers */
    str     x0, [x0]
    str     x1, [x0, #8]!
    str     x2, [x0, #8]!
    str     x3, [x0, #8]!
    str     x4, [x0, #8]!
    str     x5, [x0, #8]!
    str     x6, [x0, #8]!
    str     x7, [x0, #8]!
    str     x8, [x0, #8]!
    str     x9, [x0, #8]!
    str     x10, [x0, #8]!
    str     x11, [x0, #8]!
    str     x12, [x0, #8]!
    str     x13, [x0, #8]!
    str     x14, [x0, #8]!
    str     x15, [x0, #8]!
    str     x16, [x0, #8]!
    str     x17, [x0, #8]!
    str     x18, [x0, #8]!
    str     x19, [x0, #8]!
    str     x20, [x0, #8]!
    str     x21, [x0, #8]!
    str     x22, [x0, #8]!
    str     x23, [x0, #8]!
    str     x24, [x0, #8]!
    str     x25, [x0, #8]!
    str     x26, [x0, #8]!
    str     x27, [x0, #8]!
    str     x28, [x0, #8]!

    str     x29, [x0, #8]! /* Frame Pointer */
    str     x30, [x0, #8]! /* Link Register */

    mov     x21, sp
    str     x21, [x0, #8]! /* Stack Pointer */

    str     x30, [x0, #8]! /* Exception Link register */

    mrs     x21, CurrentEL
    cmp     x21, #0
    bne     finish
    ret

finish:
    mrs     x21, ttbr0_el1
    str     x21, [x0, #8]! /* TTBR0 */
    ret

.globl LDST_RESTORESP
LDST_RESTORESP:
    mov sp, x1
    b LDST

.globl LDST
LDST:
    str     x0, [sp, #-8]
    ldp     x2, x3, [x0, #24]!
    ldp     x4, x5, [x0, #16]!
    ldp     x6, x7, [x0, #16]!
    ldp     x8, x9, [x0, #16]!
    ldp     x10, x11, [x0, #16]!
    ldp     x12, x13, [x0, #16]!
    ldp     x14, x15, [x0, #16]!
    ldp     x16, x17, [x0, #16]!
    ldp     x18, x19, [x0, #16]!
    ldp     x20, x21, [x0, #16]!
    ldp     x22, x23, [x0, #16]!
    ldp     x24, x25, [x0, #16]!
    ldp     x26, x27, [x0, #16]!
    ldp     x28, x29, [x0, #16]!
    ldr     x30, [x0, #8]! /* Link Register */

    ldr     x1, [x0, #8]!  /* Stack Pointer */
    str     x1, [sp, #-16]

    mrs     x1, CurrentEl
    ands    x1, x1, #0xC
    cmp     x1, #0x4
    beq     system_el1

system_el2:
    ldr     x1, [x0, #8]!
    msr     elr_el2, x1   /* Exception Link Register */

    ldr     x1, [x0, #8]!
    msr     ttbr0_el1, x1

    ldr     x1, [x0, #8]!
    msr     spsr_el2, x1    /* Status Register */

    ldr     x0, [sp, #-8]  /* Recover x0 and x1 */ 
    and     x1, x1, #7
    cmp     x1, #0
    bne     1f
    ldr     x1, [sp, #-16]
    msr     sp_el0, x1      /* Recover stack pointer */
    b       end
1:
    ldr     x1, [sp, #-16]
    msr     sp_el1, x1          /* Recover stack pointer */

    b end
system_el1:
    ldr     x1, [x0, #8]!
    msr     elr_el1, x1   /* Exception Link Register */

    ldr     x1, [x0, #8]!
    msr     ttbr0_el1, x1

    ldr     x1, [x0, #8]!
    msr     spsr_el1, x1    /* Status Register */

    ldr     x0, [sp, #-8]  /* Recover x0 and x1 */ 

    and     x1, x1, #7
    cmp     x1, #0
    bne     2f
    ldr     x1, [sp, #-16]
    msr     sp_el0, x1      /* Recover stack pointer */
    b       end
2:
    ldr     x1, [sp, #-16]
    mov     sp, x1          /* Recover stack pointer */

end:
    ldr     x1, [x0, #8]
    ldr     x0, [x0]
    eret


.global pushRegisters
pushRegisters:
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
    str   x28, [sp, #-16]!
    // Do not save x29 and 30, (frame pointer and link register) as they are corrupted by the branch instruction
    ret

.global popRegisters
popRegisters:
    // Do not load x29 and 30, (frame pointer and link register) as they were corrupted by the branch instruction
    ldr   x28, [sp], #16
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
    ret

.global RIGVBAR
RIGVBAR:
    mrs    x0, vbar_el1
    orr    x0, x0, 0xFFFF000000000000
    msr    vbar_el1, x0
    ret

.globl STST_asm_el2
STST_asm_el2:
    /* General purpose registers */
    stp     x0, x1, [x5]
    stp     x2, x3, [x5, #16]!
    ldr     x0, [sp], #16
    stp     x4, x0, [x5, #16]!
    stp     x6, x7, [x5, #16]!
    stp     x8, x9, [x5, #16]!
    stp     x10, x11, [x5, #16]!
    stp     x12, x13, [x5, #16]!
    stp     x14, x15, [x5, #16]!
    stp     x16, x17, [x5, #16]!
    stp     x18, x19, [x5, #16]!
    stp     x20, x21, [x5, #16]!
    stp     x22, x23, [x5, #16]!
    stp     x24, x25, [x5, #16]!
    stp     x26, x27, [x5, #16]!
    str     x28, [x5, #16]!

    ldp     x1, x2, [sp], #16
    str     x1, [x5, #8]! /* Frame Pointer */
    str     x2, [x5, #8]! /* Link Register */

    mrs     x1, spsr_el1
    and     x1, x1, #7
    cmp     x1, #0
    bne     1f

    mrs     x1, sp_el0
    b       2f
1:
    mrs     x1, sp_el1
2:
    mrs     x2, elr_el1
    mrs     x3, ttbr0_el1
    mrs     x4, spsr_el1

    str     x1, [x5, #8]!
    str     x2, [x5, #8]!
    str     x3, [x5, #8]!
    str     x4, [x5, #8]!
    ret


.globl STST_asm
STST_asm:
    /* General purpose registers */
    stp     x0, x1, [x28]
    stp     x2, x3, [x28, #16]!
    stp     x4, x5, [x28, #16]!
    stp     x6, x7, [x28, #16]!
    stp     x8, x9, [x28, #16]!
    stp     x10, x11, [x28, #16]!
    stp     x12, x13, [x28, #16]!
    stp     x14, x15, [x28, #16]!
    stp     x16, x17, [x28, #16]!
    stp     x18, x19, [x28, #16]!
    stp     x20, x21, [x28, #16]!
    stp     x22, x23, [x28, #16]!
    stp     x24, x25, [x28, #16]!
    stp     x26, x27, [x28, #16]!
    ldp     x26, x27, [sp], #16
    str     x26, [x28, #16]!

    ldp     x26, x27, [sp], #16
    str     x26, [x28, #8]! /* Frame Pointer */
    str     x27, [x28, #8]! /* Link Register */

    mrs     x0,CurrentEL
    cmp     x0, #4
    beq     1f

    mrs     x1, spsr_el2
    mrs     x2, elr_el2
    mrs     x3, sp_el1
    b       2f
1:
    mrs     x1, spsr_el1
    mrs     x2, elr_el1
    mov     x3, sp

2:
    mov     x27, x1//spsr_el1
    and     x27, x27, #7
    cmp     x27, #0
    bne     savesp_el1_asm
    mrs     x27, sp_el0
    b       finish_asm
savesp_el1_asm:
    mov     x27, x3 //sp
    add     x27, x27, #16
finish_asm:
    str     x27, [x28, #8]!
    
    mov     x27, x2//elr_el1
    str     x27, [x28, #8]! // Note: the 16 immediate value refers to the previously saved pair

    //mrs   x27, ttbr0_el1
    ldr     x27, [sp], #16
    str     x27, [x28, #8]!

    mov     x27, x1//spsr_el1
    str     x27, [x28, #8]!
    ret

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