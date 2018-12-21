#include "system.h"
#include "bios_const.h"
#include "libuarm.h"
#include "interrupts.h"
#include "gpio.h"
#include "mailbox.h"
#include "timers.h"
#include "uart.h"
#include "asmlib.h"
#include "mmu.h"
#include "emulated_terminals.h"
#include "emulated_tapes.h"

uint64_t next_timer                 = 0;
uint64_t f_emulated_timer_interrupt = 0;

void set_next_timer(uint64_t microseconds) {
    uint8_t *interrupt_lines  = (uint8_t *)INTERRUPT_LINES;
    next_timer                = get_us() + microseconds;
    interrupt_lines[IL_TIMER] = 0;
    if (microseconds < 100) {
        setTimer(microseconds);
    }
}

uint32_t c_swi_handler(uint32_t code, uint32_t *registers) {
    state_t *state;
    switch (code) {
        case SYS_GETCURRENTEL:
            return GETSAVEDEL();
        case SYS_GETCURRENTSTATUS:
            return GETSAVEDSTATUS();
        case SYS_GETTTBR0:
            return GETTTBR0();
        case SYS_ENABLEIRQ:
            enable_irq_el0();
            uart0_puts("interrupts enabled!\n");
            return 0;
        case SYS_INITARMTIMER:
            initArmTimer();
            uart0_puts("arm timer enabled!\n");
            return 0;
        case SYS_SETNEXTTIMER:
            return setTimer((long unsigned int)registers);
        case SYS_GETSPEL0:
            return GETSP_EL0();
        case SYS_LAUNCHSTATE:
            state = (state_t *)registers;
            LDST_EL0(state);
            return 0;
        default:
            uart0_puts("Unrecognized code: ");
            hexstring(code);
            hexstring(GETSAVEDSTATUS());
            break;
    }
    return 0;
}

void c_irq_handler() {
    static uint8_t  f_led       = 0;
    uint8_t         f_interrupt = 0;
    uint32_t        tmp;
    static uint64_t lastBlink = 0;
    uint64_t        timer, handler_present;
    int             i;
    void (*interrupt_handler)();
    termreg_t *         terminal;
    tapereg_t *         tape;
    uint8_t *           interrupt_lines             = (uint8_t *)INTERRUPT_LINES;
    uint8_t             interrupt_mask              = *((uint8_t *)INTERRUPT_MASK);
    static unsigned int old_tape_command[MAX_TAPES] = {0};

    handler_present = *((uint64_t *)INTERRUPT_HANDLER);

    timer = get_us();

    /* Blink running led on real hardware */
    if (timer / 1000 - lastBlink >= 1000) {
        led(f_led);
        f_led     = f_led == 0 ? 1 : 0;
        lastBlink = timer / 1000;
    }

    tmp = *((volatile uint32_t *)CORE0_IRQ_SOURCE);

    if (tmp & (1 << 8)) {
        // TODO: uart interrupt. to be managed
    }

    for (i = 0; i < MAX_TAPES; i++) {
        tape = (tapereg_t *)DEV_REG_ADDR(IL_TAPE, i);

        switch (tape->command) {
            case RESET:
                tape->status = DEVICE_READY;
                break;
            case ACK:
                if (tape->command == old_tape_command[i])
                    break;

                old_tape_command[i] = ACK;
                tape->status        = DEVICE_READY;
                interrupt_lines[IL_TAPE] &= ~(1 << i);
                break;
            case READBLK:
                if (tape->status == DEVICE_READY) {
                    if (tape->command == old_tape_command[i])
                        break;
                    old_tape_command[i] = READBLK;
                    tape->status        = DEVICE_BUSY;
                } else if (tape->status == DEVICE_BUSY) {
                    read_tape_block(i, (unsigned char *)(uint64_t)tape->data0);
                    tape->status = DEVICE_READY;
                    if (!(interrupt_mask & (1 << IL_TAPE))) {
                        interrupt_lines[IL_TAPE] |= 1 << i;
                    }
                }
                break;
            case WRITEBLK:
                if (tape->status == DEVICE_READY) {
                    if (tape->command == old_tape_command[i])
                        break;
                    old_tape_command[i] = WRITEBLK;
                    tape->status        = DEVICE_BUSY;
                } else if (tape->status == DEVICE_BUSY) {
                    write_tape_block(i, (unsigned char *)(uint64_t)tape->data0);
                    tape->status = DEVICE_READY;
                    if (!(interrupt_mask & (1 << IL_TAPE))) {
                        interrupt_lines[IL_TAPE] |= 1 << i;
                    }
                }
        }
    }

    /* Check emulated devices */
    for (i = 0; i < MAX_TERMINALS; i++) {
        terminal = (termreg_t *)DEV_REG_ADDR(IL_TERMINAL, i);

        switch (terminal->transm_command & 0xFF) {
            case RESET:
                terminal->transm_command = RESET;
                terminal->transm_status  = DEVICE_READY;
                /* Reset command also removes interrupt */
                interrupt_lines[0] &= ~(1 << i);
                break;
            case ACK:
                terminal->transm_command = RESET;
                terminal->transm_status  = DEVICE_READY;
                interrupt_lines[IL_TERMINAL] &= ~(1 << i);
                break;
            case TRANSMIT_CHAR:
                if ((terminal->transm_status & 0xFF) == DEVICE_READY) {
                    terminal->transm_status = DEVICE_BUSY;
                    // TODO: there might be sync problems between processes due to this system
                    // I need to check if it's meant to be like that or I should avoid them
                } else if ((terminal->transm_status & 0xFF) == DEVICE_BUSY) {
                    terminal_send(i, terminal->transm_command >> 8);
                    terminal->transm_status = CHAR_TRANSMIT;
                    if (!(interrupt_mask & (1 << IL_TERMINAL))) {
                        interrupt_lines[IL_TERMINAL] |= 1 << i;
                    }
                }
                break;
        }
    }

    if (tmp & 0x08) {
        if (next_timer > 0 && timer >= next_timer) {
            if (!(interrupt_mask & (1 << IL_TIMER))) {
                interrupt_lines[IL_TIMER] = 1;
            }
            setTimer(100);
        } else if (next_timer - timer < 100) {
            setTimer(next_timer - timer);
        } else if (next_timer - timer >= 100) {
            setTimer(100);
        }
    }

    for (i = 0; i < IL_LINES; i++) {
        if (interrupt_lines[i]) {
            f_interrupt = 1;
            /* Until there are interrupt lines pending fire interrupts immediately */
            setTimer(0);
            break;
        }
    }

    if (f_interrupt && handler_present != 0) {
        interrupt_handler = (void (*)(void *))handler_present;
        interrupt_handler();
    }

    /*else {
        if (tmp & (1 << 8)) {
            // apparently not needed for real hw
            //        if (IRQ_CONTROLLER->IRQ_basic_pending & (1 << 9)) {
            if (IRQ_CONTROLLER->IRQ_pending_2 & (1 << 25)) {
                if (UART0->MASKED_IRQ & (1 << 4)) {
                    c = (unsigned char)UART0->DATA;     // read for clear tx interrupt.
                    uart0_putc(c);
                }
                if (UART0->MASKED_IRQ & (1 << 5)) {
                    uart0_puts("tx?\n");
                }
                //         }
            }
        }
    }*/
}


void c_abort_handler(uint64_t exception_code, uint64_t iss) {
    switch (exception_code) {
        case 0x24:     // Data abort (MMU)
            uart0_puts("Data abort (lower exception level) caused by ");
            if (iss & 0b1000000)
                uart0_puts("write\n");
            else
                uart0_puts("read\n");
            break;
        case 0x25:     // Data abort (MMU)
            uart0_puts("Data abort (same exception level) caused by ");
            if (iss & 0b1000000)
                uart0_puts("write\n");
            else
                uart0_puts("read\n");
            break;
        default:
            break;
    }

    hexstring(exception_code);
    hexstring(iss);
    while (1) {
        HALT();
    }
}