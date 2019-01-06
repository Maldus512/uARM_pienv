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
    /*uart0_puts("current time: ");
    hexstring(get_us());
    uart0_puts("next timer: ");
    hexstring(next_timer);*/
    if (microseconds < 100) {
        setTimer(microseconds);
    }
}

uint32_t c_swi_handler(uint32_t code, uint32_t *registers) {
    uint64_t handler_present;
    void (*synchronous_handler)(unsigned int, unsigned int, unsigned int, unsigned int);
    handler_present = *((uint64_t *)SYNCHRONOUS_HANDLER);

    if (handler_present != 0) {
        synchronous_handler = (void (*)(unsigned int, unsigned int, unsigned int, unsigned int))handler_present;
        synchronous_handler(code, registers[0], registers[1], registers[2]);
    }
    
    return 0;
}

void c_irq_handler() {
    static uint8_t  f_led       = 0;
    uint8_t         f_app_interrupt = 1;
    uint32_t        tmp;
    static uint64_t lastBlink = 0;
    uint64_t        timer, handler_present;
    int             i;
    void (*interrupt_handler)();
    uint8_t *    interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint8_t      interrupt_mask  = *((uint8_t *)INTERRUPT_MASK);
    unsigned int core_id;

    handler_present = *((uint64_t *)INTERRUPT_HANDLER);

    timer = get_us();

    /* Blink running led on real hardware */
    if (timer / 1000 - lastBlink >= 1000) {
        led(f_led);
        f_led     = f_led == 0 ? 1 : 0;
        lastBlink = timer / 1000;
    }

    core_id = GETCOREID();
    if (core_id == 0) {
        tmp = *((volatile uint32_t *)CORE0_IRQ_SOURCE);
    } else if (core_id == 1) {
        tmp = *((volatile uint32_t *)CORE1_IRQ_SOURCE);
        uart0_puts("interrupt!") ;
        hexstring(tmp);
        if (tmp & 0x08)
            setTimer(5000 * 1000);
        if (tmp & 0x10) {
            *((uint32_t*) CORE1_MBOX0_CLEARSET) = 0xFFFFFFFF;
        }

        return;
    } else {
        return;
    }

    if (tmp & (1 << 8)) {
        // TODO: uart interrupt. to be managed
        // apparently not needed for real hw
        //        if (IRQ_CONTROLLER->IRQ_basic_pending & (1 << 9)) {
        if (IRQ_CONTROLLER->IRQ_pending_2 & (1 << 25)) {
            if (UART0->MASKED_IRQ & (1 << 4)) {
                nop();
            }
            if (UART0->MASKED_IRQ & (1 << 5)) {
                nop();
            }
        }
    }

    for (i = 0; i < MAX_TAPES; i++) {
        manage_emulated_tape(i);
    }

    /* Check emulated devices */
    for (i = 0; i < MAX_TERMINALS; i++) {
        manage_emulated_terminal(i);
    }

    if (tmp & 0x08) {
        if (next_timer > 0 && timer >= next_timer) {
            if (!(interrupt_mask & (1 << IL_TIMER))) {
                interrupt_lines[IL_TIMER] = 1;
            }
            setTimer(100);
        } else if (next_timer - timer < 100) {
            f_app_interrupt = 0;
            setTimer(next_timer - timer);
        } else if (next_timer - timer >= 100) {
            f_app_interrupt = 0;
            setTimer(100);
        }
    }

    for (i = 0; i < IL_LINES; i++) {
        if (interrupt_lines[i] && !(interrupt_mask & (1 << i))) {
            /* Until there are interrupt lines pending fire interrupts immediately */
            setTimer(0);
            break;
        }
    }

    if (f_app_interrupt && handler_present != 0) {
        interrupt_handler = (void (*)(void *))handler_present;
        interrupt_handler();
    }
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