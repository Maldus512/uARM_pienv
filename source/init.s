.global _start
.global _get_stack_pointer
.global _enable_interrupts

.equ    CPSR_MODE_USER,         0x10
.equ    CPSR_MODE_FIQ,          0x11
.equ    CPSR_MODE_IRQ,          0x12
.equ    CPSR_MODE_SVR,          0x13
.equ    CPSR_MODE_ABORT,        0x17
.equ    CPSR_MODE_UNDEFINED,    0x1B
.equ    CPSR_MODE_SYSTEM,       0x1F

.equ    CPSR_IRQ_INHIBIT,       0x80
.equ    CPSR_FIQ_INHIBIT,       0x40
.equ    CPSR_THUMB,             0x20


.section .init
_start:
    ldr pc, _reset_h
    ldr pc, _undefined_instruction_vector_h
    ldr pc, _software_interrupt_vector_h
    ldr pc, _prefetch_abort_vector_h
    ldr pc, _data_abort_vector_h
    ldr pc, _unused_handler_h
    ldr pc, _interrupt_vector_h
    ldr pc, _fast_interrupt_vector_h

_reset_h:                           .word   _reset
_undefined_instruction_vector_h:    .word   stub_vector
_software_interrupt_vector_h:       .word   _swi_handler
_prefetch_abort_vector_h:           .word   stub_vector
_data_abort_vector_h:               .word   stub_vector
_unused_handler_h:                  .word   _reset
_interrupt_vector_h:                .word   irq
_fast_interrupt_vector_h:           .word   stub_vector

_reset:
/*
    // We start on hypervisor mode. Switch back to SVC
    mrs r0,cpsr
    bic r0,r0,#0x1F
    orr r0,r0,#0x13
    msr spsr_cxsf,r0
    add r0,pc,#4
    msr ELR_hyp,r0
    eret*/

    // Move the exception vector
    mov     r0, #0x8000
    mov     r1, #0x0000
    ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
    stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}
    ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
    stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}

    // We're going to use interrupt mode, so setup the interrupt mode
    // stack pointer which differs to the application stack pointer:
    mov r0, #(CPSR_MODE_IRQ | CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT )
    msr cpsr_c, r0
    mov sp, #(63 * 1024 * 1024)

    // Switch back to supervisor mode (our application mode) and
    // set the stack pointer towards the end of RAM. Remember that the
    // stack works its way down memory, our heap will work it's way
    // up memory toward the application stack.
    mov r0, #(CPSR_MODE_SVR | CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT )
    msr cpsr_c, r0

    // Set the stack pointer at some point in RAM that won't harm us
    // It's different from the IRQ stack pointer above and no matter
    // what the GPU/CPU memory split, 64MB is available to the CPU
    // Keep it within the limits and also keep it aligned to a 32-bit
    // boundary!
    mov     sp, #(64 * 1024 * 1024)

    bl _crt0

_hang:
    bl _hang

_get_stack_pointer:
    // Return the stack pointer value
    str     sp, [sp]
    ldr     r0, [sp]
    // Return from the function
    mov     pc, lr


_enable_interrupts:
    mrs     r0, cpsr
    bic     r0, r0, #0x80
    msr     cpsr_c, r0

    mov     pc, lr



irq:
    push {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    bl c_irq_handler
    pop  {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
    subs pc,lr,#4


_swi_handler:
    push     {r0-r12,lr}       //; Store registers.
    mov      r1, sp               //; Set pointer to parameters.
    mrs      r0, spsr             //; Get SPSR.
    push     {r0,r3}              //; Store SPSR onto stack and another register to maintain
                                  //; 8-byte-aligned stack. Only required for nested SVCs.
    tst      r0,#0x20             //; Occurred in Thumb state?
    ldrneh   r0,[lr,#-2]          //; Yes: load halfword and...
    bicne    r0,r0,#0xFF00        //; ...extract comment field.
    ldreq    r0,[lr,#-4]          //; No: load word and...
    biceq    r0,r0,#0xFF000000    //; ...extract comment field.
                                  //; R0 now contains SVC number
                                  //; R1 now contains pointer to stacked registers
    bl       c_swi_handler        //; Call C routine to handle the SVC.
    pop      {r0,r3}              //; Get SPSR from stack.
    msr      spsr_cf, r0          //; Restore SPSR.
    ldm      sp!, {r0-r12,pc}^ //; Restore registers and return.
    // the '^' indicates to copy spsr in cpsr (if the register list contains the pc)
    // it is effectively the return from the exception. Not to use in user or system mode

.global PANIC
PANIC:
//TODO: WARNING: CAUTION: we have to push lr and spsr onto the stack only if 
// we already are in svc mode.
    mrs     r0, spsr
    push    {r0, lr}
    swi     #0x2  // PANIC code
    pop     {r0, lr}
    msr     spsr, r0
    mov     pc, lr

/*.global WAIT
WAIT:
    wfi
    mov     pc, lr
    */

.global HALT
HALT:
//TODO: WARNING: CAUTION: we have to push lr and spsr onto the stack only if 
// we already are in svc mode.
    mrs     r0, spsr
    push    {r0, lr}
    swi     #0x1  // HALT code
    pop     {r0, lr}
    msr     spsr, r0
    mov     pc, lr

.end