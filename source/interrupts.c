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
#include "emulated_timers.h"

uint64_t f_emulated_timer_interrupt = 0;
uint64_t wait_lock                  = 0;

void set_next_timer(uint64_t microseconds) {
    uint8_t *interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint64_t currentTime     = get_us();
    uint64_t timer           = currentTime + microseconds;
    timer_t  next;

    interrupt_lines[IL_TIMER] = 0;

    add_timer(timer, TIMER, 0);
    if (next_timer(&next) == 0) {
        if (currentTime < next.time)
            setTimer(next.time - currentTime);
        else
            setTimer(0);
    }
}

void set_device_timer(uint64_t microseconds, TIMER_TYPE type, int device_num) {
    uint64_t currentTime     = get_us();
    uint64_t timer           = currentTime + microseconds;
    timer_t  next;

    add_timer(timer, type, device_num);
    if (next_timer(&next) == 0) {
        if (currentTime < next.time)
            setTimer(next.time - currentTime);
        else
            setTimer(0);
    }
}



void c_fiq_handler() {
    uint32_t tmp, data, device_num, device_class;
    uint64_t address;
    tmp = GIC->Core0_FIQ_Source;

    GIC->Core0_MailBox0_ClearSet = 0xFFFFFFFF;
    uart0_puts("FIQ!\n");
    return;

    if (tmp & 0x10) {
        data         = GIC->Core0_MailBox0_ClearSet;
        address      = (uint64_t)(data & ~0xF);
        device_class = data & 0x0000000C;
        device_num   = data & 0x00000003;

        switch (device_class >> 2) {
            case DEVICE_CLASS_PRINTER:
                emulated_printer_mailbox(device_num, (printreg_t *)address);
                break;
            case DEVICE_CLASS_TAPE:
                emulated_tape_mailbox(device_num, (tapereg_t *)address);
                break;
        }


        GIC->Core0_MailBox0_ClearSet = data;
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
    static uint8_t  f_led           = 0;
    uint8_t         f_app_interrupt = 1;
    uint32_t        tmp;
    static uint64_t lastBlink = 0;
    uint64_t        currentTime, handler_present;
    int             i, res;
    void (*interrupt_handler)();
    uint8_t *    interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint8_t      interrupt_mask  = *((uint8_t *)INTERRUPT_MASK);
    unsigned int core_id;
    timer_t      next;

    handler_present = *((uint64_t *)INTERRUPT_HANDLER);

    currentTime = get_us();

    /* Blink running led on real hardware */
    if (currentTime / 1000 - lastBlink >= 1000) {
        led(f_led);
        f_led     = f_led == 0 ? 1 : 0;
        lastBlink = currentTime / 1000;
    }

    core_id = GETCOREID();
    if (core_id == 0) {
        tmp = GIC->Core0_IRQ_Source;
    } else if (core_id == 1) {
        tmp = GIC->Core1_IRQ_Source;
        uart0_puts("interrupt!");
        hexstring(tmp);
        if (tmp & 0x08)
            setTimer(5000 * 1000);
        if (tmp & 0x10) {
            GIC->Core1_MailBox0_ClearSet = 0xFFFFFFFF;
        }

        return;
    } else {
        return;
    }

    if (tmp & (1 << 8)) {
        // TODO: uart interrupt. to be managed
        unsigned int temp = MU_IIR;

        if ((temp & 0x01) == 0) {
            //MU_IIR = temp;
            nop();
        }

        // apparently not needed for real hw
        //        if (IRQ_CONTROLLER->IRQ_basic_pending & (1 << 9)) {
        if (IRQ_CONTROLLER->IRQ_pending_2 & (1 << 25)) {
            if (UART0->MASKED_IRQ & (1 << 4)) {
                nop();
                //UART0->IRQ_CLEAR = 0xFFF;
            }
            if (UART0->MASKED_IRQ & (1 << 5)) {
                nop();
                //UART0->IRQ_CLEAR = 0xFFF;
            }
        }
    }


    if (tmp & 0x08) {
        /* More precise timing */
        currentTime = get_us();

        while ((res = next_pending_timer(currentTime, &next)) > 0) {
            switch (next.type) {
                /* Don't clear the timer interrupt on purpose; it's on the user to to that */
                case TIMER:
                    interrupt_lines[IL_TIMER] = 1;
                    break;
                case TAPE:
                    manage_emulated_tape(next.code);
                    break;
                case PRINTER:
                    manage_emulated_printer(next.code);
                    break;
                case DISK:
                    break;
                case UNALLOCATED:
                    break;
            }
        }

        if (res == 0) {
            setTimer(next.time-currentTime);
        }
    }

    for (i = 0; i < IL_LINES; i++) {
        if (interrupt_lines[i] && !(interrupt_mask & (1 << i))) {
            /* Until there are interrupt lines pending fire interrupts immediately */
            setTimer(0);
            f_app_interrupt = 1;
            break;
        }
    }

    if (f_app_interrupt && handler_present != 0) {
        wait_lock         = 1;
        interrupt_handler = (void (*)(void *))handler_present;
        interrupt_handler();
    }
}


void c_abort_handler(uint64_t exception_code, uint64_t iss) {
    switch (exception_code) {
        case 0x24:     // Data abort (MMU)
            uart0_puts("Data abort (lower exception level) caused by ");
            if (iss & 0x40)
                uart0_puts("write\n");
            else
                uart0_puts("read\n");
            break;
        case 0x25:     // Data abort (MMU)
            uart0_puts("Data abort (same exception level) caused by ");
            if (iss & 0x40)
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