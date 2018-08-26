#include "interrupts.h"
#include "gpio.h"
#include "timers.h"
#include "uart.h"
#include "bios_const.h"
#include "asmlib.h"
#include "libuarmv2.h"


void _halt() {
    while (1) {
        asm volatile ("wfe");
    }
}

void _wait() {
    asm volatile("wfi");
}


void stub_vector() {
    while(1) {
        nop();
    }
}

uint32_t c_swi_handler(uint32_t code, uint32_t *registers)
{
    switch (code) {
        case SYS_GETARMCLKFRQ:
            return GETARMCLKFRQ();
        case SYS_GETARMCOUNTER:
            return GETARMCOUNTER();

        default:
            uart_puts("ciao\n");
            hexstring(GETEL());
            hexstring(GETSAVEDSTATE());
            break;

    }
    return 0;
}



void  c_irq_handler() {
    static uint8_t f_led = 0;

    unsigned int rb;
    int i = 0;

    if (IRQ_CONTROLLER->IRQ_pending_1 & (1 << 29)) { // Mini UART
        while(i++ < 1000) //resolve all interrupts to uart
        {
            rb=MU_IIR;
            if((rb&1)==1) break; //no more interrupts
            if((rb&6)==4)
            {
                //receiver holds a valid byte
                RxBuffer[rx_head++] = MU_IO & 0xFF; //read byte from rx fifo
                rx_head = rx_head % MU_RX_BUFFER_SIZE;
            }
        }
    }

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
}