#include "interrupts.h"
#include "gpio.h"
#include "timers.h"
#include "uart.h"



void stub_vector() {
    while(1) {
        nop();
    }
}



void  __attribute__((interrupt("IRQ")))  c_irq_handler() {
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