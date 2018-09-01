#include "interrupts.h"
#include "gpio.h"
#include "timers.h"
#include "uart.h"
#include "bios_const.h"
#include "asmlib.h"
#include "libuarmv2.h"


uint32_t c_swi_handler(uint32_t code, uint32_t *registers)
{
    switch (code) {
        case SYS_GETARMCLKFRQ:
            return GETARMCLKFRQ();
        case SYS_GETARMCOUNTER:
            return GETARMCOUNTER();
        case SYS_ENABLEIRQ:
            tprint("interrupts enabled!\n");
            enable_irq();
            return 0;

        default:
            uart0_puts("ciao\n");
            hexstring(GETEL());
            hexstring(GETSAVEDSTATE());
            break;

    }
    return 0;
}

void c_irq_handler(void)
{
    char c;
    unsigned int rb;
    disable_irq();
    // check inteerupt source
    hexstring(IRQ_CONTROLLER->IRQ_basic_pending);
    hexstring(IRQ_CONTROLLER->IRQ_pending_1);
    hexstring(IRQ_CONTROLLER->IRQ_pending_2);
    if (*CORE0_INTERRUPT_SOURCE & (1 << 8)) {
        if (IRQ_CONTROLLER->IRQ_basic_pending & (1 << 9)) {
            if (IRQ_CONTROLLER->IRQ_pending_2 & (1 << 25)) {
                if (UART0->MASKED_IRQ & (1 << 4)) {
                    c = (unsigned char) UART0->DATA; // read for clear tx interrupt.
                    uart0_putc(c);
                    uart0_puts(" c_irq_handler\n");
                    return;
                }
            }
        }
    }

    enable_irq();
    return;

    if (IRQ_CONTROLLER->IRQ_pending_1 & (1 << 29)) { // Mini UART
        while(1) //resolve all interrupts to uart
        {
            rb=MU_IIR;
            if((rb&1)==1) break; //no more interrupts
            if((rb&6)==4)
            {
                //receiver holds a valid byte
                RxBuffer[rx_head++] = MU_IO & 0xFF; //read byte from rx fifo
                rx_head = rx_head % MU_RX_BUFFER_SIZE;
            }
            else
                break;
        }
    }
}



// TODO: it doesn't work on real hardware, only on qemu
void startUart0Int() {
    // enable UART RX interrupt.
    UART0->IRQ_MASK = 1 << 4;

    // UART interrupt routing.
    IRQ_CONTROLLER->Enable_IRQs_2 |= 1 << 25;
    //*IRQ_ENABLE2 = 1 << 25;

    // IRQ routeing to CORE0.
    *GPU_INTERRUPTS_ROUTING = 0x00;

    enable_irq();
}

/*void  c_irq_handler() {
    static uint8_t f_led = 0;



    if (IRQ_CONTROLLER->IRQ_basic_pending & 0x1) { // ARM TIMER
        ARMTIMER->IRQCLEAR = 1;
        if (f_led) {
            setGpio(21);
        } else {
            clearGpio(21);
        }
        f_led = 1-f_led;
        if (f_led != 1 && f_led != 0) {
            f_led = 0;
        }
    }
}*/