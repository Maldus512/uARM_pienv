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
#include "utils.h"
#include "emulated_printers.h"
#include "emulated_tapes.h"
#include "emulated_disks.h"
#include "emulated_timers.h"

uint64_t wait_lock                    = 0;
uint64_t scheduled_physical_timers[4] = {0};

// TODO: clean up
int pending_emulated_interrupt() {
    uint8_t *interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint8_t  interrupt_mask  = *((uint8_t *)INTERRUPT_MASK);
    int      i;
    for (i = 0; i < IL_LINES; i++) {
        if (interrupt_lines[i] && !(interrupt_mask & (1 << i))) {
            /* Until there are interrupt lines pending fire interrupts immediately */
            return 1;
        }
    }
    return 0;
}

int pending_real_interrupt(int core) {
    // Ignore physical timer interrupt, it is managed as an emulated device
    switch (core) {
        case 0:
            return GIC->Core0_IRQ_Source & 0xFFFD ? 1 : 0;
            break;
        case 1:
            return GIC->Core1_IRQ_Source & 0xFFFD ? 1 : 0;
            break;
        case 2:
            return GIC->Core2_IRQ_Source & 0xFFFD ? 1 : 0;
            break;
        case 3:
            return GIC->Core3_IRQ_Source & 0xFFFD ? 1 : 0;
            break;

        default:
            return 0;
            break;
    }
}

void clean_interrupt_lines() {
    uint8_t *interrupt_lines   = (uint8_t *)INTERRUPT_LINES;
    uint8_t *installed_devices = (uint8_t *)DEVICE_INSTALLED;
    int      i;
    for (i = 0; i < IL_LINES; i++) {
        interrupt_lines[i] = interrupt_lines[i] & installed_devices[i];
    }
}

void setTIMER(uint64_t microseconds) {
    uint8_t *    interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    uint64_t     currentTime     = getTOD();
    uint64_t     timer           = currentTime + microseconds;
    unsigned int core            = getCORE();

    interrupt_lines[IL_TIMER]       = 0;
    scheduled_physical_timers[core] = timer;

    // Only set the next timer if there are no pending interrupt lines
    if (currentTime < scheduled_physical_timers[core] && !pending_emulated_interrupt()) {
        set_physical_timer(scheduled_physical_timers[core] - currentTime);
    } else {
        set_physical_timer(0);
    }
}

void set_device_timer(uint64_t microseconds, TIMER_TYPE type, int device_num) {
    uint64_t currentTime = getTOD();
    uint64_t timer       = currentTime + microseconds;
    timer_t  next;

    add_timer(timer, type, device_num);
    if (next_timer(&next) == 0) {
        if (currentTime < next.time)
            set_virtual_timer(next.time - currentTime);
        else
            set_virtual_timer(0);
    }
}



void c_fiq_handler() {
    uint32_t tmp, data, device_num, device_class;
    uint64_t address, currentTime;
    int      res;
    timer_t  next;

    tmp = GIC->Core0_FIQ_Source;

    if (tmp & 0x10) {
        data         = GIC->Core0_MailBox0_ClearSet;
        address      = (uint64_t)(data & ~0xF);
        device_class = data & 0x0000000C;
        device_num   = data & 0x00000003;

        switch (device_class >> 2) {
            case DEVICE_CLASS_DISK:
                emulated_disk_mailbox(device_num, (diskreg_t *)address);
                break;
            case DEVICE_CLASS_PRINTER:
                emulated_printer_mailbox(device_num, (printreg_t *)address);
                break;
            case DEVICE_CLASS_TAPE:
                emulated_tape_mailbox(device_num, (tapereg_t *)address);
                break;
        }

        // clear interrupt
        GIC->Core0_MailBox0_ClearSet = data;
    }

    if (tmp & 0x08) {
        currentTime = getTOD();

        while ((res = next_pending_timer(currentTime, &next)) > 0) {
            switch (next.type) {
                case TAPE:
                    manage_emulated_tape(next.code);
                    break;
                case PRINTER:
                    manage_emulated_printer(next.code);
                    break;
                case DISK:
                    manage_emulated_disk(next.code);
                    break;
                case UNALLOCATED:
                    break;
            }
        }

        // If there are more pending timers, set the first one
        if (res == 0) {
            set_virtual_timer(next.time - currentTime);
        } else {
            disable_virtual_counter();
        }

        if (pending_emulated_interrupt()) {
            /* Until there are interrupt lines pending fire interrupts immediately */
            set_physical_timer(0);
        }
    }
    clean_interrupt_lines();
}

void c_swi_handler(uint32_t code, uint32_t *registers) {
    state_t  kernel;
    uint64_t handler_present, stack_pointer;
    void (*synchronous_handler)(unsigned int, unsigned int, unsigned int, unsigned int);
    uint32_t core_id;
    core_id = getCORE();

    if (ISMMUACTIVE())
        handler_present = *((uint64_t *)SYNCHRONOUS_HANDLER) | 0xFFFF000000000000;
    else
        handler_present = *((uint64_t *)SYNCHRONOUS_HANDLER);

    if (handler_present != 0) {
        // If we already were at EL1 do not change stack pointer
        if (GETSAVEDEL() == 0)
            stack_pointer = *((uint64_t *)(KERNEL_CORE0_SP + 0x8 * core_id));
        else
            stack_pointer = GETSP();

        synchronous_handler  = (void (*)(unsigned int, unsigned int, unsigned int, unsigned int))handler_present;
        kernel.stack_pointer = stack_pointer;
        kernel.exception_link_register = (uint64_t)synchronous_handler;
        kernel.status_register         = 0x385;
        kernel.TTBR0                   = GETTTBR0();
        LDST(&kernel);
    }

    /* If there is no user-defined handler simply start again the last process */
    LDST((void *)(SYNCHRONOUS_OLDAREA + CORE_OFFSET * core_id));
}

void c_irq_handler() {
    static uint8_t  f_led     = 0;
    static uint64_t lastBlink = 0;

    state_t  kernel, *oldarea;
    uint64_t currentTime, handler_present, stack_pointer;
    void (*interrupt_handler)();
    uint8_t *    interrupt_lines = (uint8_t *)INTERRUPT_LINES;
    unsigned int tmp, core_id;

    if (ISMMUACTIVE())
        handler_present = *((uint64_t *)INTERRUPT_HANDLER) | 0xFFFF000000000000;
    else
        handler_present = *((uint64_t *)INTERRUPT_HANDLER);

    currentTime = getTOD();
    core_id     = getCORE();

    /* Core 0 manages all interrupts */
    if (core_id == 0) {
        tmp = GIC->Core0_IRQ_Source;

        /* Blink running led on real hardware */
        if (currentTime / 1000 - lastBlink >= 1000) {
            led(f_led);
            f_led     = f_led == 0 ? 1 : 0;
            lastBlink = currentTime / 1000;
        }
    }

    /* A timer interrupt means one of two things: either a timer interrupt
    reached its condition or there are emulated interrupt lines pending
    and I want to make sure they are managed */
    if (tmp & 0x02) {
        /* More precise timing */
        currentTime = getTOD();

        if (currentTime >= scheduled_physical_timers[core_id]) {
            /* Don't clear the timer interrupt on purpose; it's on the user to to that */
            interrupt_lines[IL_TIMER]          = 1;
            scheduled_physical_timers[core_id] = 0;
            disable_physical_counter();
        } else {
            set_physical_timer(scheduled_physical_timers[core_id] - currentTime);
        }
    }

    if (pending_emulated_interrupt()) {
        /* Until there are interrupt lines pending fire interrupts immediately */
        set_physical_timer(0);
    }

    clean_interrupt_lines();

    oldarea = (state_t *)(INTERRUPT_OLDAREA + CORE_OFFSET * core_id);

    if (handler_present != 0 && (pending_emulated_interrupt() || pending_real_interrupt(core_id))) {
        wait_lock                      = 1;
        stack_pointer                  = *((uint64_t *)(KERNEL_CORE0_SP + 0x8 * core_id));
        interrupt_handler              = (void (*)(void *))handler_present;
        kernel.stack_pointer           = stack_pointer;
        kernel.exception_link_register = (uint64_t)interrupt_handler;
        kernel.status_register         = 0x385;
        kernel.TTBR0                   = GETTTBR0();
        LDST(&kernel);
    }
    /* If there is no user-defined handler simply start again the last process */
    LDST(oldarea);
}


void c_abort_handler(uint64_t exception_code, uint64_t iss) {
    state_t kernel;
    char string[64];
    void (*interrupt_handler)();
    uint64_t     handler_present, stack_pointer;
    unsigned int core_id;
    core_id = getCORE();

    if (ISMMUACTIVE())
        handler_present = *((uint64_t *)ABORT_HANDLER) | 0xFFFF000000000000;
    else
        handler_present = *((uint64_t *)ABORT_HANDLER);

    if (handler_present) {
        // If we already were at EL1 do not change stack pointer
        if (GETSAVEDEL() == 0)
            stack_pointer = *((uint64_t *)(KERNEL_CORE0_SP + 0x8 * core_id));
        else
            stack_pointer = GETSP();

        interrupt_handler              = (void (*)(void *))handler_present;
        kernel.stack_pointer           = stack_pointer;
        kernel.exception_link_register = (uint64_t)interrupt_handler;
        kernel.status_register         = 0x385;
        kernel.TTBR0                   = GETTTBR0();
        LDST(&kernel);
    } else {
        switch (exception_code) {
            case 0x20:
            case 0x21:
                LOG(WARN, "Instruction Abort");
                break;
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
                LOG(WARN, "Data abort:");
                break;
        }

        itoa(exception_code, string, 16);
        LOG(INFO, string);
        itoa(iss, string, 16);
        LOG(INFO, string);
        while (1) {
            HALT();
        }
    }
}