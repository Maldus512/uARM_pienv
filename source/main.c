#include "uart.h"
#include "hardwareprofile.h"
#include "gpio.h"
#include "mailbox.h"

#ifdef APP
extern void main();
#endif


void initSystem() {
    initGpio();
    initUart();
    uart_puts("************************************\n");
    uart_puts("*        MaldOS running...         *\n");
    uart_puts("************************************\n");
}


void bios_main()
{
    initSystem();

    led(1);
    
    #ifdef APP
    main();
    #endif
    
    // echo everything back
    while(1) {
        uart_putc(uart_getc());
    }
}
