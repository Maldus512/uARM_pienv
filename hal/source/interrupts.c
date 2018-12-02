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

uint64_t emulated_timer             = 0;
uint64_t next_timer                 = 0;
uint64_t f_emulated_timer_interrupt = 0;

void setNextTimer(uint64_t milliseconds) { next_timer = emulated_timer + milliseconds; }

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
    static uint8_t                   f_led       = 0;
    uint8_t                          f_interrupt = 0;
    uint32_t                         tmp;
    static uint64_t                  lastBlink = 0;
    __attribute__((unused)) state_t *state;
    uint64_t                         timer, handler_present;
    void (*interrupt_handler)();

    handler_present = *((uint64_t *)INTERRUPT_HANDLER);

    timer = getMillisecondsSinceStart();

    if (timer - lastBlink >= 1000) {
        led(f_led);
        f_led     = f_led == 0 ? 1 : 0;
        lastBlink = timer;
    }

    tmp = *((volatile uint32_t *)CORE0_IRQ_SOURCE);
    if (tmp & 0x08) {
        setTimer(1);
        emulated_timer++;

        if (next_timer > 0 && emulated_timer >= next_timer) {
            f_interrupt = 1;
        }
    }
    if (tmp & (1 << 8)) {
        f_interrupt = 1;
    }

    if (f_interrupt && handler_present != 0) {
        interrupt_handler = (void (*)(void *))handler_present;
        interrupt_handler();
    }

    /*else {
        if (tmp & 0x08) {
            setTimer(1000);
            uart0_puts("int\n");
        }

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
    return;
}