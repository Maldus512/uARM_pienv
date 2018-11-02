#include "uart.h"
#include "hardwareprofile.h"
#include "gpio.h"
#include "mailbox.h"
#include "asmlib.h"
#include "rand.h"
#include "timers.h"
#include "libuarm.h"
#include "libuarmv2.h"
#include "interrupts.h"
#include "uARMtypes.h"

#ifdef APP
extern void main();
#endif


void initSystem() {
    initGpio();
    initUart0();
    initRand();
    SYSCALL(SYS_INITARMTIMER,0,0,0);
    SYSCALL(SYS_ENABLEIRQ,0,0,0);
    tprint("************************************\n");
    tprint("*        MaldOS running...         *\n");
    tprint("************************************\n");
}

void systemCheckup() {
    uint32_t serial[2];
    int i;
    int x;
    state_t test;
    STST(&test);

    x = GETEL();//SYSCALL(SYS_GETCURRENTEL, 0,0,0);
    hexstring(x);

    tprint("Turning on LED RUN and blink...\n");
    setGpio(LED_RUN);
    led(1);

    serialNumber(serial);
    tprint("My serial number is:\n");
    hexstring(serial[0]);
    hexstring(serial[1]);

    for (i = 0; i <3; i++) {
        delay_us(100*1000);
        tprint("blink - ");
        setGpio(LED_RUN);
        led(0);
        delay_us(100*1000);
        clearGpio(LED_RUN);
        led(1);
    }
    tprint("\n");

    clearGpio(LED_RUN);
    led(0);
    tprint("check if I return from an interrupt\n");
    DOWFI();
    setGpio(LED_RUN);
    led(1);

    tprint("System ready!\n");
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
        uart0_putc(uart0_getc());
    }
}