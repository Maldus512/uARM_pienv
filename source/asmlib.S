#include "bios_const.h"

.globl GETEL
GETEL:
    mrs x0,CurrentEL
    ret

.globl GETSAVEDEL
GETSAVEDEL:
    mov x0, #0
    mrs x0, spsr_el1
    and x0, x0, #0x7
    ret

.globl GETSAVEDSTATUS
GETSAVEDSTATUS:
    mrs x0,spsr_el1
    ret

.globl GETTTBR0
GETTTBR0:
    mrs x0, ttbr0_el1
    ret

.global wait_lock
.global WAIT
WAIT:
    dsb ish
    mov  x1, 0
    ldr  x0, =wait_lock
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

.global GETARMCOUNTER
GETARMCOUNTER:
    //mrs x0, cntpct_el0
    mrs x0, cntvct_el0
    ret


.globl PANIC
PANIC:
    wfe
    b PANIC


.globl GETSP_EL0
GETSP_EL0:
    mrs    x0, sp_el0
    ret

.globl GETCOREID
GETCOREID:
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

.globl LDST
LDST:
    str     x0, [sp, #-8]
    ldr     x2, [x0, #16]!
    ldr     x3, [x0, #8]!
    ldr     x4, [x0, #8]!
    ldr     x5, [x0, #8]!
    ldr     x6, [x0, #8]!
    ldr     x7, [x0, #8]!
    ldr     x8, [x0, #8]!
    ldr     x9, [x0, #8]!
    ldr     x10, [x0, #8]!
    ldr     x11, [x0, #8]!
    ldr     x12, [x0, #8]!
    ldr     x13, [x0, #8]!
    ldr     x14, [x0, #8]!
    ldr     x15, [x0, #8]!
    ldr     x16, [x0, #8]!
    ldr     x17, [x0, #8]!
    ldr     x18, [x0, #8]!
    ldr     x19, [x0, #8]!
    ldr     x20, [x0, #8]!
    ldr     x21, [x0, #8]!
    ldr     x22, [x0, #8]!
    ldr     x23, [x0, #8]!
    ldr     x24, [x0, #8]!
    ldr     x25, [x0, #8]!
    ldr     x26, [x0, #8]!
    ldr     x27, [x0, #8]!
    ldr     x28, [x0, #8]!

    ldr     x29, [x0, #8]! /* frame pointer */
    ldr     x30, [x0, #8]! /* Link Register */

    ldr     x1, [x0, #8]!  /* Stack Pointer */
    str     x1, [sp, #-16]
    //mov     sp, x1
    //msr     sp_el0, x1

    ldr     x1, [x0, #8]!
    msr     elr_el1, x1   /* Exception Link Register */

    ldr     x1, [x0, #8]!
    msr     ttbr0_el1, x1 

    ldr     x1, [x0, #8]!
    msr     spsr_el1, x1    /* Status Register */

    ldr     x0, [sp, #-8]  /* Recover x0 and x1 */ 

    and     x1, x1, #7
    cmp     x1, #0
    bne     el1_sp
    ldr     x1, [sp, #-16]
    msr     sp_el0, x1      /* Recover stack pointer */
    b       end
el1_sp:
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

.global vbarForMMU
vbarForMMU:
    mrs    x0, vbar_el1
    orr    x0, x0, 0xFFFF000000000000
    msr    vbar_el1, x0
    ret


.globl STST_asm
STST_asm:
    /* General purpose registers */
    str     x0, [x28]
    str     x1, [x28, #8]!
    str     x2, [x28, #8]!
    str     x3, [x28, #8]!
    str     x4, [x28, #8]!
    str     x5, [x28, #8]!
    str     x6, [x28, #8]!
    str     x7, [x28, #8]!
    str     x8, [x28, #8]!
    str     x9, [x28, #8]!
    str     x10, [x28, #8]!
    str     x11, [x28, #8]!
    str     x12, [x28, #8]!
    str     x13, [x28, #8]!
    str     x14, [x28, #8]!
    str     x15, [x28, #8]!
    str     x16, [x28, #8]!
    str     x17, [x28, #8]!
    str     x18, [x28, #8]!
    str     x19, [x28, #8]!
    str     x20, [x28, #8]!
    str     x21, [x28, #8]!
    str     x22, [x28, #8]!
    str     x23, [x28, #8]!
    str     x24, [x28, #8]!
    str     x25, [x28, #8]!
    str     x26, [x28, #8]!
    str     x27, [x28, #8]!
    str     x28, [x28, #8]!

    str     x26, [x28, #8]! /* Frame Pointer */
    str     x27, [x28, #8]! /* Link Register */

    mrs   x27, spsr_el1
    and   x27, x27, #7
    cmp   x27, #0
    bne   savesp_el1_asm
    mrs   x27, sp_el0
    b     finish_asm
savesp_el1_asm:
    mov   x27, sp
finish_asm:
    str   x27, [x28, #8]!
    
    mrs   x27, elr_el1
    str   x27, [x28, #8]! // Note: the 16 immediate value refers to the previously saved pair

    mrs   x27, ttbr0_el1
    str   x27, [x28, #8]!

    mrs   x27, spsr_el1
    str   x27, [x28, #8]!
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