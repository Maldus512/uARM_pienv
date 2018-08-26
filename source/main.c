#include "uart.h"
#include "hardwareprofile.h"
#include "gpio.h"
#include "mailbox.h"
#include "asmlib.h"

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
    setGpio(LED_RUN);
    led(1);

    //hexstring(GETEL());

    //asm volatile ("svc #0");
    SYSCALL();

    uart_puts("returned from syscall\n");
    
    #ifdef APP
    main();
    #endif

    // echo everything back
    while(1) {
        uart_putc(uart_getc());
    }
}
