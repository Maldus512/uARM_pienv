#include "interrupts.h"
#include "gpio.h"
#include "timers.h"
#include "uart.h"
#include "bios_const.h"


void _halt() {
    while (1) {
        asm volatile ("wfe");
    }
}


void stub_vector() {
    while(1) {
        nop();
    }
}

void c_swi_handler(uint32_t code, uint32_t *registers)
{
    switch (code) {
        case BIOS_SRV_HALT:
            uart_puts("HALT\n");
            _halt();
            break;
        case BIOS_SRV_PANIC:
            uart_puts("KERNEL PANIC!\n");
            _halt();
            break;
    }
}



void  c_irq_handler() {
    static uint8_t led = 0;

    unsigned int rb;

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
        }
    }

    if (IRQ_CONTROLLER->IRQ_basic_pending & 0x1) { // ARM TIMER
        ARMTIMER->IRQCLEAR = 1;
        if (led) {
            setGpio(4);
        } else {
            clearGpio(4);
        }
        led = 1-led;
    }
}