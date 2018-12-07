#include "arch.h"
#include "bios_const.h"
#include "uart.h"
#include "gpio.h"
#include "mailbox.h"
#include "asmlib.h"
#include "timers.h"
#include "libuarm.h"
#include "interrupts.h"
#include "mmu.h"
#include "sd.h"
#include "system.h"
#include "emulated_devices.h"

#ifdef APP
extern void main();
#endif

uint64_t ciao[256];


extern int __uMPS_stack;

void initSystem() {
    termreg_t *terminal;
    int i = 0;
    *((uint64_t *)INTERRUPT_HANDLER)   = 0;
    *((uint64_t *)SYNCHRONOUS_HANDLER) = 0;

    for (i = 0; i < MAX_TERMINALS; i++) {
        terminal = (termreg_t*) DEV_REG_ADDR(i, 0);
        terminal->transm_status = DEVICE_READY;
        terminal->recv_status = DEVICE_READY;
    }

    initGpio();
    initUart0();
   /* uart0_puts("************************************\n");
    uart0_puts("*        MaldOS running...         *\n");
    uart0_puts("************************************\n");
    */

    SYSCALL(SYS_INITARMTIMER, 0, 0, 0);
    SYSCALL(SYS_SETNEXTTIMER, 1, 0, 0);
}

void systemCheckup() {
    uint32_t serial[2];

    hexstring(*((uint32_t *)SYSCALL(SYS_GETCURRENTEL, 0, 0, 0)));

    uart0_puts("Turning on LED RUN and blink...\n");
    setGpio(LED_RUN);
    led(1);

    serialNumber(serial);
    uart0_puts("My serial number is:\n");
    hexstring(serial[0]);
    hexstring(serial[1]);


    uart0_puts("System ready!\n");
}

void bios_main() {
    initSystem();

    lfb_init();
    lfb_print(10, 10, "MaldOS start");
#ifdef APP
    main();
#endif

    // echo everything back
    while (1) {
        uart0_putc(uart0_getc());
    }
}
