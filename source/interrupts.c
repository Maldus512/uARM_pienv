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

uint64_t emulated_timer             = 0;
uint64_t device_timer               = 0;
uint64_t f_emulated_timer_interrupt = 0;
uint64_t wait_lock                  = 0;

void set_next_timer(uint64_t microseconds) {
    uint8_t *interrupt_lines  = (uint8_t *)INTERRUPT_LINES;
    emulated_timer            = get_us() + microseconds;
    interrupt_lines[IL_TIMER] = 0;
    if (device_timer == 0 || emulated_timer < device_timer) {
        setTimer(microseconds);
    }
}

void set_device_timer(uint64_t microseconds) {
    device_timer = get_us() + microseconds;
    if (emulated_timer == 0 || device_timer < emulated_timer) {
        setTimer(microseconds);
    }
}



void c_fiq_handler() {
    uint32_t tmp, data, device_num, device_class;
    uint64_t address;
    tmp = GIC->Core0_FIQ_Source;

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
    for (i = 0; i < MAX_PRINTERS; i++) {
        manage_emulated_printer(i);
    }

    if (tmp & 0x08) {
        /* More precise timing */
        timer = get_us();
        /* Don't clear the timer interrupt on purpose; it's on the user to to that */
        if (emulated_timer > 0 && timer >= emulated_timer) {
            if (!(interrupt_mask & (1 << IL_TIMER))) {
                interrupt_lines[IL_TIMER] = 1;
            }
            /*if (device_timer > 0 && timer < device_timer) {
                setTimer(device_timer);
            }*/
        }
        if (device_timer > 0 && timer >= device_timer) {
            // TODO: implement a proper timer queue
            device_timer = 0;
            if (emulated_timer > 0 && timer < emulated_timer) {
                setTimer(emulated_timer - timer);
            }
        } else if (device_timer > 0) {
            setTimer(device_timer - timer);
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