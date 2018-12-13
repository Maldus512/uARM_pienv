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

uint64_t next_timer                 = 0;
uint64_t f_emulated_timer_interrupt = 0;

void setNextTimer(uint64_t microseconds) {
    next_timer = getMicrosecondsSinceStart() + microseconds;
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
        case SYS_INITMMU:
            initMMU();
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
    termreg_t *terminal;
    uint8_t *  interrupt_lines = (uint8_t *)INTERRUPT_LINES;

    handler_present = *((uint64_t *)INTERRUPT_HANDLER);

    timer = getMicrosecondsSinceStart();

    /* Blink running led on real hardware */
    if (timer / 1000 - lastBlink >= 1000) {
        led(f_led);
        f_led     = f_led == 0 ? 1 : 0;
        lastBlink = timer / 1000;
    }

    tmp = *((volatile uint32_t *)CORE0_IRQ_SOURCE);

    if (tmp & (1 << 8)) {
        f_interrupt = 1;
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
                interrupt_lines[0] &= ~(1 << i);
                break;
            case TRANSMIT_CHAR:
                if ((terminal->transm_status & 0xFF) == DEVICE_READY) {
                    terminal->transm_status = DEVICE_BUSY;
                    //TODO: there might be sync problems between processes due to this system
                    // I need to check if it's meant to be like that or I should avoid them
                } else if ((terminal->transm_status & 0xFF) == DEVICE_BUSY) {
                    terminal_send(i, terminal->transm_command >> 8);
                    terminal->transm_status = CHAR_TRANSMIT;
                    interrupt_lines[0] |= 1 << i;
                    // f_interrupt = 1;
                }
                break;
        }
    }

    if (tmp & 0x08) {
        if (next_timer > 0 && timer >= next_timer) {
            f_interrupt = 1;
            setTimer(100);
        } else if (next_timer - timer < 100) {
            setTimer(next_timer - timer);
        } else if (next_timer - timer >= 100) {
            setTimer(100);
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