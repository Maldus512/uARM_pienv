#include "uart.h"
#include "hardwareprofile.h"
#include "gpio.h"
#include "mailbox.h"
#include "asmlib.h"
#include "rand.h"
#include "timers.h"
#include "libuarm.h"

#ifdef APP
extern void main();
#endif


void initSystem() {
    initGpio();
    initUart();
    initRand();
    uart_puts("************************************\n");
    uart_puts("*        MaldOS running...         *\n");
    uart_puts("************************************\n");
}

void systemCheckup() {
    uint32_t serial[2];
    int i;

    uart_puts("Turning on LED RUN and blink...\n");
    setGpio(LED_RUN);
    led(1);

    //delay_us(1000*1000);

    SYSCALL(0,0,0,0);
    uart_puts("returned from syscall\n");

    //delay_us(1000*1000);

    serialNumber(serial);
    uart_puts("My serial number is:\n");
    hexstring(serial[0]);
    hexstring(serial[1]);

    uart_puts("Ten random numbers\n");
    for (i = 0; i < 10; i++) {
        hexstring(rand(0,0xFFFFFFFF));
    }

    for (i = 0; i <3; i++) {
        delay_us(500*1000);
        uart_puts("blink - ");
        setGpio(LED_RUN);
        led(0);
        delay_us(500*1000);
        clearGpio(LED_RUN);
        led(1);
    }

    uart_puts("System ready!\n");
}


void bios_main()
{
    initSystem();
    systemCheckup();
    
    #ifdef APP
    main();
    #endif

    // echo everything back
    while(1) {
        uart_putc(uart_getc());
    }
}
