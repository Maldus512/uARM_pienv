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
#include "arch.h"
#include "mmu.h"

#ifdef APP
extern void main();
#endif

uint64_t ciao[256];

void test() {
    tprint("printing stuff\n");
    //uint32_t x = SYSCALL(SYS_GETCURRENTEL, 0, 0, 0);
    while (1) {
        ciao[24] = 512;
        tprint("alive : ");
        hexstring(getTODHI());
        hexstring(getTODLO());
        delay_us(1000 * 1000);
    }
}

extern int __uMPS_stack;

void initSystem() {
    /* Memory info expected by uMPS */
    *((uint32_t *)BUS_REG_RAM_BASE) = (uint64_t)&__uMPS_stack;
    *((uint32_t *)BUS_REG_RAM_SIZE) = 4096;
    initGpio();
    initUart0();
    initRand();
    startUart0Int();
    tprint("************************************\n");
    tprint("*        MaldOS running...         *\n");
    tprint("************************************\n");
}

void systemCheckup() {
    uint32_t serial[2];

    hexstring(*((uint32_t *)SYSCALL(SYS_GETCURRENTEL,0,0,0)));

    tprint("Turning on LED RUN and blink...\n");
    setGpio(LED_RUN);
    led(1);

    serialNumber(serial);
    tprint("My serial number is:\n");
    hexstring(serial[0]);
    hexstring(serial[1]);

    SYSCALL(SYS_INITARMTIMER, 0, 0, 0);
    SYSCALL(SYS_ENABLEIRQ, 0, 0, 0);
    SYSCALL(SYS_SETNEXTTIMER, 1000,0,0);

    tprint("System ready!\n");
}


void bios_main() {
    initSystem();
    systemCheckup();

#ifdef APP
    main();
#else
    test();
#endif

    // echo everything back
    while (1) {
        uart0_putc(uart0_getc());
    }
}
